#include "repositories/AccountRepository.h"

#include <memory>
#include <utility>

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
}

AccountRepository::AccountRepository(DatabaseManager& database)
    : m_database(database)
{
}

bool AccountRepository::add(Account& account, std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot add account: database is not open.";
        return false;
    }

    if (account.getId() != 0)
    {
        error = "A new account must have ID 0.";
        return false;
    }

    const char* sql =
        "INSERT INTO accounts "
        "(name, type, initial_balance_paise) "
        "VALUES (?, ?, ?);";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    sqlite3_bind_text(
        statement.get(),
        1,
        account.getName().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement.get(),
        2,
        account.getType().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int64(
        statement.get(),
        3,
        account.getInitialBalancePaise()
    );

    const int result = sqlite3_step(statement.get());

    if (result != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    const sqlite3_int64 newId =
        sqlite3_last_insert_rowid(m_database.handle());

    account.setId(static_cast<int>(newId));

    error.clear();
    return true;
}

std::optional<Account> AccountRepository::findById(
    int id,
    std::string& error) const
{
    if (!m_database.isOpen())
    {
        error = "Cannot find account: database is not open.";
        return std::nullopt;
    }

    if (id <= 0)
    {
        error = "Account ID must be positive.";
        return std::nullopt;
    }

    const char* sql =
        "SELECT id, name, type, initial_balance_paise "
        "FROM accounts WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return std::nullopt;
    }

    sqlite3_bind_int(statement.get(), 1, id);

    const int result = sqlite3_step(statement.get());

    if (result == SQLITE_DONE)
    {
        error = "Account not found.";
        return std::nullopt;
    }

    if (result != SQLITE_ROW)
    {
        error = sqlite3_errmsg(m_database.handle());
        return std::nullopt;
    }

    Account account(
        sqlite3_column_int(statement.get(), 0),
        columnText(statement.get(), 1),
        columnText(statement.get(), 2),
        sqlite3_column_int64(statement.get(), 3)
    );

    error.clear();
    return account;
}

std::vector<Account> AccountRepository::getAll(
    std::string& error) const
{
    std::vector<Account> accounts;

    if (!m_database.isOpen())
    {
        error = "Cannot retrieve accounts: database is not open.";
        return accounts;
    }

    const char* sql =
        "SELECT id, name, type, initial_balance_paise "
        "FROM accounts ORDER BY id;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return accounts;
    }

    int result;

    while ((result = sqlite3_step(statement.get())) == SQLITE_ROW)
    {
        accounts.emplace_back(
            sqlite3_column_int(statement.get(), 0),
            columnText(statement.get(), 1),
            columnText(statement.get(), 2),
            sqlite3_column_int64(statement.get(), 3)
        );
    }

    if (result != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        accounts.clear();
        return accounts;
    }

    error.clear();
    return accounts;
}

bool AccountRepository::update(
    const Account& account,
    std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot update account: database is not open.";
        return false;
    }

    if (account.getId() <= 0)
    {
        error = "Account ID must be positive.";
        return false;
    }

    const char* sql =
        "UPDATE accounts "
        "SET name = ?, type = ?, initial_balance_paise = ? "
        "WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    sqlite3_bind_text(
        statement.get(),
        1,
        account.getName().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement.get(),
        2,
        account.getType().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int64(
        statement.get(),
        3,
        account.getInitialBalancePaise()
    );

    sqlite3_bind_int(statement.get(), 4, account.getId());

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_changes(m_database.handle()) == 0)
    {
        error = "Account not found or no changes were made.";
        return false;
    }

    error.clear();
    return true;
}

bool AccountRepository::remove(int id, std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot delete account: database is not open.";
        return false;
    }

    if (id <= 0)
    {
        error = "Account ID must be positive.";
        return false;
    }

    const char* sql = "DELETE FROM accounts WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    sqlite3_bind_int(statement.get(), 1, id);

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        return false;
    }

    if (sqlite3_changes(m_database.handle()) == 0)
    {
        error = "Account not found.";
        return false;
    }

    error.clear();
    return true;
}
