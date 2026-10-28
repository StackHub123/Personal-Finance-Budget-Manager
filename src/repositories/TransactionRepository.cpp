#include "repositories/TransactionRepository.h"

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
    using Statement =
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

    bool prepareStatement(
        sqlite3* database,
        const char* sql,
        Statement& statement,
        std::string& error)
    {
        sqlite3_stmt* rawStatement = nullptr;

        const int result = sqlite3_prepare_v2(
            database,
            sql,
            -1,
            &rawStatement,
            nullptr
        );

        statement.reset(rawStatement);

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        return true;
    }

    std::string columnText(sqlite3_stmt* statement, int column)
    {
        const auto* value = sqlite3_column_text(statement, column);

        if (value == nullptr)
        {
            return {};
        }

        return reinterpret_cast<const char*>(value);
    }

    const char* typeToString(TransactionType type)
    {
        switch (type)
        {
            case TransactionType::Income:
                return "income";

            case TransactionType::Expense:
                return "expense";
        }

        return nullptr;
    }

    bool stringToType(
        const std::string& value,
        TransactionType& type)
    {
        if (value == "income")
        {
            type = TransactionType::Income;
            return true;
        }

        if (value == "expense")
        {
            type = TransactionType::Expense;
            return true;
        }

        return false;
    }

    bool bindText(
        sqlite3_stmt* statement,
        int index,
        const std::string& value,
        sqlite3* database,
        std::string& error)
    {
        const int result = sqlite3_bind_text(
            statement,
            index,
            value.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        return true;
    }

    bool bindTransactionFields(
        sqlite3_stmt* statement,
        const Transaction& transaction,
        sqlite3* database,
        std::string& error)
    {
        const char* type = typeToString(transaction.getType());

        if (type == nullptr)
        {
            error = "Invalid transaction type.";
            return false;
        }

        int result = sqlite3_bind_int(
            statement, 1, transaction.getAccountId());

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        result = sqlite3_bind_int64(
            statement, 2, transaction.getAmountPaise());

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        if (!bindText(statement, 3, type, database, error) ||
            !bindText(statement, 4, transaction.getCategory(),
                      database, error) ||
            !bindText(statement, 5, transaction.getDescription(),
                      database, error) ||
            !bindText(statement, 6, transaction.getDate(),
                      database, error))
        {
            return false;
        }

        return true;
    }

    bool readTransaction(
        sqlite3_stmt* statement,
        Transaction& transaction)
    {
        TransactionType type;

        if (!stringToType(columnText(statement, 3), type))
        {
            return false;
        }

        try
        {
            transaction = Transaction(
                sqlite3_column_int(statement, 0),
                sqlite3_column_int(statement, 1),
                sqlite3_column_int64(statement, 2),
                type,
                columnText(statement, 4),
                columnText(statement, 5),
                columnText(statement, 6)
            );
        }
        catch (const std::exception&)
        {
            return false;
        }

        return true;
    }
}

TransactionRepository::TransactionRepository(DatabaseManager& database)
    : m_database(database)
{
}

bool TransactionRepository::add(
    Transaction& transaction,
    std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot add transaction: database is not open.";
        return false;
    }

    if (transaction.getId() != 0)
    {
        error = "A new transaction must have ID 0.";
        return false;
    }

    const char* sql =
        "INSERT INTO transactions "
        "(account_id, amount_paise, type, category, description, "
        "transaction_date) VALUES (?, ?, ?, ?, ?, ?);";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    if (!bindTransactionFields(
            statement.get(), transaction, m_database.handle(), error))
    {
        return false;
    }

    const int result = sqlite3_step(statement.get());

    if (result != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    const sqlite3_int64 newId =
        sqlite3_last_insert_rowid(m_database.handle());

    if (newId <= 0 ||
        newId > std::numeric_limits<int>::max())
    {
        error = "Generated transaction ID is outside the supported range.";
        return false;
    }

    transaction.setId(static_cast<int>(newId));

    error.clear();
    return true;
}

std::optional<Transaction> TransactionRepository::findById(
    int id,
    std::string& error) const
{
    if (!m_database.isOpen())
    {
        error = "Cannot find transaction: database is not open.";
        return std::nullopt;
    }

    if (id <= 0)
    {
        error = "Transaction ID must be positive.";
        return std::nullopt;
    }

    const char* sql =
        "SELECT id, account_id, amount_paise, type, category, "
        "description, transaction_date "
        "FROM transactions WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return std::nullopt;
    }

    const int bindResult = sqlite3_bind_int(statement.get(), 1, id);

    if (bindResult != SQLITE_OK)
    {
        error = sqlite3_errmsg(m_database.handle());
        return std::nullopt;
    }

    const int result = sqlite3_step(statement.get());

    if (result == SQLITE_DONE)
    {
        error = "Transaction not found.";
        return std::nullopt;
    }

    if (result != SQLITE_ROW)
    {
        error = sqlite3_errmsg(m_database.handle());
        return std::nullopt;
    }

    Transaction transaction(
        0, 1, 1, TransactionType::Expense,
        "placeholder", "placeholder", "2000-01-01"
    );

    if (!readTransaction(statement.get(), transaction))
    {
        error = "Stored transaction contains invalid data.";
        return std::nullopt;
    }

    error.clear();
    return transaction;
}

std::vector<Transaction> TransactionRepository::getAll(
    std::string& error) const
{
    std::vector<Transaction> transactions;

    if (!m_database.isOpen())
    {
        error = "Cannot retrieve transactions: database is not open.";
        return transactions;
    }

    const char* sql =
        "SELECT id, account_id, amount_paise, type, category, "
        "description, transaction_date "
        "FROM transactions ORDER BY id;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return transactions;
    }

    int result;

    while ((result = sqlite3_step(statement.get())) == SQLITE_ROW)
    {
        Transaction transaction(
            0, 1, 1, TransactionType::Expense,
            "placeholder", "placeholder", "2000-01-01"
        );

        if (!readTransaction(statement.get(), transaction))
        {
            error = "A stored transaction contains invalid data.";
            transactions.clear();
            return transactions;
        }

        transactions.push_back(transaction);
    }

    if (result != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        transactions.clear();
        return transactions;
    }

    error.clear();
    return transactions;
}

bool TransactionRepository::update(
    const Transaction& transaction,
    std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot update transaction: database is not open.";
        return false;
    }

    if (transaction.getId() <= 0)
    {
        error = "Transaction ID must be positive.";
        return false;
    }

    const char* sql =
        "UPDATE transactions SET "
        "account_id = ?, amount_paise = ?, type = ?, category = ?, "
        "description = ?, transaction_date = ? "
        "WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    if (!bindTransactionFields(
            statement.get(), transaction, m_database.handle(), error))
    {
        return false;
    }

    const int bindResult = sqlite3_bind_int(
        statement.get(), 7, transaction.getId());

    if (bindResult != SQLITE_OK)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_changes(m_database.handle()) == 0)
    {
        error = "Transaction not found or no changes were made.";
        return false;
    }

    error.clear();
    return true;
}

bool TransactionRepository::remove(int id, std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot delete transaction: database is not open.";
        return false;
    }

    if (id <= 0)
    {
        error = "Transaction ID must be positive.";
        return false;
    }

    const char* sql = "DELETE FROM transactions WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    const int bindResult = sqlite3_bind_int(statement.get(), 1, id);

    if (bindResult != SQLITE_OK)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_changes(m_database.handle()) == 0)
    {
        error = "Transaction not found.";
        return false;
    }

    error.clear();
    return true;
}


bool TransactionRepository::addRestored(
    const Transaction& transaction,
    std::string& error)
{
    if (!m_database.isOpen()) {
        error = "Cannot restore transaction: database is not open.";
        return false;
    }

    if (transaction.getId() <= 0) {
        error = "Restored transaction ID must be positive.";
        return false;
    }

    const char* sql =
        "INSERT INTO transactions "
        "(account_id, amount_paise, type, category, description, "
        "transaction_date, id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error)) {
        return false;
    }

    // Bind the usual six transaction fields using the existing
    // repository validation and mapping helper.
    if (!bindTransactionFields(
            statement.get(), transaction, m_database.handle(), error)) {
        return false;
    }

    const int result = sqlite3_bind_int(
        statement.get(), 7, transaction.getId());

    if (result != SQLITE_OK) {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    error.clear();
    return true;
}
