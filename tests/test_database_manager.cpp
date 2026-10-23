#include "database/DatabaseManager.h"

#include <cstdio>
#include <iostream>
#include <string>

namespace
{
    bool tableExists(sqlite3* db, const std::string& tableName)
    {
        const char* sql =
            "SELECT COUNT(*) FROM sqlite_master "
            "WHERE type = 'table' AND name = ?1;";

        sqlite3_stmt* statement = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_text(
            statement,
            1,
            tableName.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        const bool exists =
            sqlite3_step(statement) == SQLITE_ROW &&
            sqlite3_column_int(statement, 0) == 1;

        sqlite3_finalize(statement);
        return exists;
    }
}

int main()
{
    const std::string path = "/tmp/pfbm_database_manager_test.db";
    std::remove(path.c_str());

    DatabaseManager database;
    std::string error;

    // Test 1: Open the database.
    if (!database.open(path, error))
    {
        std::cerr << "FAIL: Could not open database: " << error << '\n';
        return 1;
    }

    std::cout << "Test 1 passed: Database opened successfully.\n";

    // Test 2: Create the database tables.
    if (!database.initializeSchema(error))
    {
        std::cerr << "FAIL: Could not initialize schema: " << error << '\n';
        return 1;
    }

    sqlite3* db = database.handle();

    if (!tableExists(db, "accounts") ||
        !tableExists(db, "transactions") ||
        !tableExists(db, "budgets"))
    {
        std::cerr << "FAIL: One or more database tables are missing.\n";
        return 1;
    }

    std::cout << "Test 2 passed: All three tables were created.\n";

    // Test 3: Confirm that foreign keys are enabled.
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, "PRAGMA foreign_keys;", -1, &statement, nullptr) != SQLITE_OK)
    {
        std::cerr << "FAIL: Could not check foreign-key setting.\n";
        return 1;
    }

    const bool foreignKeysEnabled =
        sqlite3_step(statement) == SQLITE_ROW &&
        sqlite3_column_int(statement, 0) == 1;

    sqlite3_finalize(statement);

    if (!foreignKeysEnabled)
    {
        std::cerr << "FAIL: Foreign-key enforcement is disabled.\n";
        return 1;
    }

    std::cout << "Test 3 passed: Foreign-key enforcement is enabled.\n";

    // Create one valid account for the remaining tests.
    if (!database.execute(
            "INSERT INTO accounts (name, type, initial_balance_paise) "
            "VALUES ('Test Account', 'Cash', 100000);",
            error))
    {
        std::cerr << "FAIL: Could not insert test account: " << error << '\n';
        return 1;
    }

    // Test 4: A transaction must reference an existing account.
    const bool invalidTransactionAccepted = database.execute(
        "INSERT INTO transactions "
        "(account_id, amount_paise, type, category, description, transaction_date) "
        "VALUES (999, 1000, 'expense', 'Food', 'Test', '2026-10-23');",
        error
    );

    if (invalidTransactionAccepted)
    {
        std::cerr << "FAIL: Transaction with nonexistent account was accepted.\n";
        return 1;
    }

    std::cout << "Test 4 passed: Invalid account references were rejected.\n";

    // Test 5: Valid transactions should be accepted.
    if (!database.execute(
            "INSERT INTO transactions "
            "(account_id, amount_paise, type, category, description, transaction_date) "
            "VALUES (1, 1000, 'expense', 'Food', 'Lunch', '2026-10-23');",
            error))
    {
        std::cerr << "FAIL: Could not insert valid transaction: " << error << '\n';
        return 1;
    }

    std::cout << "Test 5 passed: Valid transaction was inserted.\n";

    // Test 6: Data should survive closing and reopening the database.
    database.close();

    if (!database.open(path, error))
    {
        std::cerr << "FAIL: Could not reopen database: " << error << '\n';
        return 1;
    }

    statement = nullptr;

    if (sqlite3_prepare_v2(
            database.handle(),
            "SELECT COUNT(*) FROM transactions;",
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        std::cerr << "FAIL: Could not query persisted transactions.\n";
        return 1;
    }

    const bool dataPersisted =
        sqlite3_step(statement) == SQLITE_ROW &&
        sqlite3_column_int(statement, 0) == 1;

    sqlite3_finalize(statement);

    if (!dataPersisted)
    {
        std::cerr << "FAIL: Transaction was not preserved after reopening.\n";
        return 1;
    }

    std::cout << "Test 6 passed: Data persisted after reopening.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll DatabaseManager tests passed!\n";
    return 0;
}
