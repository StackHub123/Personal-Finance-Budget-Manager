#include "database/DatabaseManager.h"

#include <string>

namespace
{
    bool tableHasColumn(
        sqlite3* database,
        const char* tableName,
        const char* columnName,
        bool& found,
        std::string& error)
    {
        const std::string sql = std::string("PRAGMA table_info(") + tableName + ");";
        sqlite3_stmt* statement = nullptr;

        const int prepareResult = sqlite3_prepare_v2(
            database, sql.c_str(), -1, &statement, nullptr);
        if (prepareResult != SQLITE_OK) {
            error = sqlite3_errmsg(database);
            return false;
        }

        found = false;
        int result = SQLITE_OK;
        while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
            const auto* name = sqlite3_column_text(statement, 1);
            if (name != nullptr &&
                std::string(reinterpret_cast<const char*>(name)) == columnName) {
                found = true;
            }
        }

        if (result != SQLITE_DONE) {
            error = sqlite3_errmsg(database);
            sqlite3_finalize(statement);
            return false;
        }

        sqlite3_finalize(statement);
        return true;
    }
}

DatabaseManager::DatabaseManager()
    : m_db(nullptr)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open(const std::string& path, std::string& error)
{
    if (path.empty())
    {
        error = "Database path cannot be empty.";
        return false;
    }

    // Close any existing connection before opening another.
    close();

    sqlite3* openedDatabase = nullptr;

    const int result = sqlite3_open_v2(
        path.c_str(),
        &openedDatabase,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (result != SQLITE_OK)
    {
        error = openedDatabase
                    ? sqlite3_errmsg(openedDatabase)
                    : "Failed to allocate a SQLite database connection.";

        if (openedDatabase != nullptr)
        {
            sqlite3_close_v2(openedDatabase);
        }

        return false;
    }

    m_db = openedDatabase;

    // Wait up to five seconds if another connection is using the database.
    if (sqlite3_busy_timeout(m_db, 5000) != SQLITE_OK)
    {
        error = sqlite3_errmsg(m_db);
        close();
        return false;
    }

    // SQLite foreign-key enforcement must be enabled on each connection.
    if (!execute("PRAGMA foreign_keys = ON;", error))
    {
        close();
        return false;
    }

    error.clear();
    return true;
}

void DatabaseManager::close() noexcept
{
    if (m_db != nullptr)
    {
        sqlite3_close_v2(m_db);
        m_db = nullptr;
    }
}

bool DatabaseManager::isOpen() const noexcept
{
    return m_db != nullptr;
}

bool DatabaseManager::execute(
    const std::string& sql,
    std::string& error)
{
    if (m_db == nullptr)
    {
        error = "Database is not open.";
        return false;
    }

    char* sqliteError = nullptr;

    const int result = sqlite3_exec(
        m_db,
        sql.c_str(),
        nullptr,
        nullptr,
        &sqliteError
    );

    if (result != SQLITE_OK)
    {
        error = sqliteError
                    ? sqliteError
                    : sqlite3_errmsg(m_db);

        sqlite3_free(sqliteError);
        return false;
    }

    sqlite3_free(sqliteError);
    error.clear();
    return true;
}

bool DatabaseManager::initializeSchema(std::string& error)
{
    if (!isOpen())
    {
        error = "Cannot initialize schema: database is not open.";
        return false;
    }

    // Start a transaction so schema creation can be rolled back on failure.
    if (!execute("BEGIN IMMEDIATE;", error))
    {
        return false;
    }

    const std::string schema = R"SQL(
        CREATE TABLE IF NOT EXISTS accounts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL CHECK(length(trim(name)) > 0),
            type TEXT NOT NULL CHECK(length(trim(type)) > 0),
            initial_balance_paise INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account_id INTEGER NOT NULL,
            amount_paise INTEGER NOT NULL CHECK(amount_paise > 0),
            type TEXT NOT NULL CHECK(type IN ('income', 'expense')),
            category TEXT NOT NULL CHECK(length(trim(category)) > 0),
            description TEXT NOT NULL CHECK(length(trim(description)) > 0),
            transaction_date TEXT NOT NULL,
            FOREIGN KEY (account_id)
                REFERENCES accounts(id)
                ON UPDATE CASCADE
                ON DELETE RESTRICT
        );

        CREATE INDEX IF NOT EXISTS idx_transactions_account_date
            ON transactions(account_id, transaction_date);

        CREATE INDEX IF NOT EXISTS idx_transactions_category
            ON transactions(category);

        CREATE TABLE IF NOT EXISTS budgets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account_id INTEGER,
            category TEXT NOT NULL CHECK(length(trim(category)) > 0),
            month INTEGER NOT NULL CHECK(month BETWEEN 1 AND 12),
            year INTEGER NOT NULL CHECK(year > 0),
            limit_paise INTEGER NOT NULL CHECK(limit_paise > 0),
            FOREIGN KEY (account_id)
                REFERENCES accounts(id)
                ON UPDATE CASCADE
                ON DELETE RESTRICT
        );
    )SQL";

    if (!execute(schema, error))
    {
        std::string rollbackError;
        execute("ROLLBACK;", rollbackError);
        return false;
    }

    // Existing installations have a budgets table without account_id and
    // with a global category/month/year uniqueness constraint. Rebuild that
    // table transactionally, preserving every row and ID. Old budgets remain
    // unassigned (account_id NULL) until the user chooses an account for them.
    bool hasAccountId = false;
    if (!tableHasColumn(m_db, "budgets", "account_id", hasAccountId, error)) {
        std::string rollbackError;
        execute("ROLLBACK;", rollbackError);
        return false;
    }

    if (!hasAccountId) {
        const std::string migration = R"SQL(
            DROP TABLE IF EXISTS budgets_account_migration;

            CREATE TABLE budgets_account_migration (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                account_id INTEGER,
                category TEXT NOT NULL CHECK(length(trim(category)) > 0),
                month INTEGER NOT NULL CHECK(month BETWEEN 1 AND 12),
                year INTEGER NOT NULL CHECK(year > 0),
                limit_paise INTEGER NOT NULL CHECK(limit_paise > 0),
                FOREIGN KEY (account_id)
                    REFERENCES accounts(id)
                    ON UPDATE CASCADE
                    ON DELETE RESTRICT
            );

            INSERT INTO budgets_account_migration
                (id, account_id, category, month, year, limit_paise)
            SELECT id, NULL, category, month, year, limit_paise
            FROM budgets;

            DROP TABLE budgets;
            ALTER TABLE budgets_account_migration RENAME TO budgets;
        )SQL";

        if (!execute(migration, error)) {
            std::string rollbackError;
            execute("ROLLBACK;", rollbackError);
            return false;
        }
    }

    // Separate partial indexes preserve the old duplicate protection for
    // unassigned legacy budgets while allowing identical budget periods for
    // different accounts.
    const std::string budgetIndexes = R"SQL(
        CREATE UNIQUE INDEX IF NOT EXISTS
            idx_budgets_account_category_period
        ON budgets(account_id, category COLLATE NOCASE, month, year)
        WHERE account_id IS NOT NULL;

        CREATE UNIQUE INDEX IF NOT EXISTS
            idx_budgets_unassigned_category_period
        ON budgets(category COLLATE NOCASE, month, year)
        WHERE account_id IS NULL;
    )SQL";

    if (!execute(budgetIndexes, error)) {
        std::string rollbackError;
        execute("ROLLBACK;", rollbackError);
        return false;
    }

    if (!execute("COMMIT;", error))
    {
        std::string rollbackError;
        execute("ROLLBACK;", rollbackError);
        return false;
    }

    error.clear();
    return true;
}

sqlite3* DatabaseManager::handle() const noexcept
{
    return m_db;
}
