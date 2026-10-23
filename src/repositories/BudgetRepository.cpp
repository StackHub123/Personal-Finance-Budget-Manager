#include "repositories/BudgetRepository.h"

#include <limits>
#include <memory>
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

    bool bindBudgetFields(
        sqlite3_stmt* statement,
        const Budget& budget,
        sqlite3* database,
        std::string& error)
    {
        if (!bindText(
                statement, 1, budget.getCategory(), database, error))
        {
            return false;
        }

        int result = sqlite3_bind_int(
            statement, 2, budget.getMonth());

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        result = sqlite3_bind_int(
            statement, 3, budget.getYear());

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        result = sqlite3_bind_int64(
            statement, 4, budget.getLimitPaise());

        if (result != SQLITE_OK)
        {
            error = sqlite3_errmsg(database);
            return false;
        }

        return true;
    }
}

BudgetRepository::BudgetRepository(DatabaseManager& database)
    : m_database(database)
{
}

bool BudgetRepository::add(Budget& budget, std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot add budget: database is not open.";
        return false;
    }

    if (budget.getId() != 0)
    {
        error = "A new budget must have ID 0.";
        return false;
    }

    const char* sql =
        "INSERT INTO budgets (category, month, year, limit_paise) "
        "VALUES (?, ?, ?, ?);";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    if (!bindBudgetFields(
            statement.get(), budget, m_database.handle(), error))
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
        error = "Generated budget ID is outside the supported range.";
        return false;
    }

    budget.setId(static_cast<int>(newId));

    error.clear();
    return true;
}

std::optional<Budget> BudgetRepository::findById(
    int id,
    std::string& error) const
{
    if (!m_database.isOpen())
    {
        error = "Cannot find budget: database is not open.";
        return std::nullopt;
    }

    if (id <= 0)
    {
        error = "Budget ID must be positive.";
        return std::nullopt;
    }

    const char* sql =
        "SELECT id, category, month, year, limit_paise "
        "FROM budgets WHERE id = ?;";

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
        error = "Budget not found.";
        return std::nullopt;
    }

    if (result != SQLITE_ROW)
    {
        error = sqlite3_errmsg(m_database.handle());
        return std::nullopt;
    }

    Budget budget(
        sqlite3_column_int(statement.get(), 0),
        columnText(statement.get(), 1),
        sqlite3_column_int(statement.get(), 2),
        sqlite3_column_int(statement.get(), 3),
        sqlite3_column_int64(statement.get(), 4)
    );

    error.clear();
    return budget;
}

std::vector<Budget> BudgetRepository::getAll(
    std::string& error) const
{
    std::vector<Budget> budgets;

    if (!m_database.isOpen())
    {
        error = "Cannot retrieve budgets: database is not open.";
        return budgets;
    }

    const char* sql =
        "SELECT id, category, month, year, limit_paise "
        "FROM budgets ORDER BY year, month, category;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return budgets;
    }

    int result;

    while ((result = sqlite3_step(statement.get())) == SQLITE_ROW)
    {
        budgets.emplace_back(
            sqlite3_column_int(statement.get(), 0),
            columnText(statement.get(), 1),
            sqlite3_column_int(statement.get(), 2),
            sqlite3_column_int(statement.get(), 3),
            sqlite3_column_int64(statement.get(), 4)
        );
    }

    if (result != SQLITE_DONE)
    {
        error = sqlite3_errmsg(m_database.handle());
        budgets.clear();
        return budgets;
    }

    error.clear();
    return budgets;
}

bool BudgetRepository::update(
    const Budget& budget,
    std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot update budget: database is not open.";
        return false;
    }

    if (budget.getId() <= 0)
    {
        error = "Budget ID must be positive.";
        return false;
    }

    const char* sql =
        "UPDATE budgets SET category = ?, month = ?, year = ?, "
        "limit_paise = ? WHERE id = ?;";

    Statement statement(nullptr, sqlite3_finalize);

    if (!prepareStatement(
            m_database.handle(), sql, statement, error))
    {
        return false;
    }

    if (!bindBudgetFields(
            statement.get(), budget, m_database.handle(), error))
    {
        return false;
    }

    const int bindResult = sqlite3_bind_int(
        statement.get(), 5, budget.getId());

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
        error = "Budget not found or no changes were made.";
        return false;
    }

    error.clear();
    return true;
}

bool BudgetRepository::remove(int id, std::string& error)
{
    if (!m_database.isOpen())
    {
        error = "Cannot delete budget: database is not open.";
        return false;
    }

    if (id <= 0)
    {
        error = "Budget ID must be positive.";
        return false;
    }

    const char* sql = "DELETE FROM budgets WHERE id = ?;";

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
        error = "Budget not found.";
        return false;
    }

    error.clear();
    return true;
}
