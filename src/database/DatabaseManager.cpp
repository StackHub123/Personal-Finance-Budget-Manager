#include "database/DatabaseManager.h"

#include <string>

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
            category TEXT NOT NULL CHECK(length(trim(category)) > 0),
            month INTEGER NOT NULL CHECK(month BETWEEN 1 AND 12),
            year INTEGER NOT NULL CHECK(year > 0),
            limit_paise INTEGER NOT NULL CHECK(limit_paise > 0),
            UNIQUE(category COLLATE NOCASE, month, year)
        );
    )SQL";

    if (!execute(schema, error))
    {
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
