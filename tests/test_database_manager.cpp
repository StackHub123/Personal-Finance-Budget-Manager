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

    // Test 7: Migrate an existing database with the legacy budgets schema.
    const std::string legacyPath = "/tmp/pfbm_database_manager_legacy_test.db";
    std::remove(legacyPath.c_str());
    DatabaseManager legacyDatabase;

    if (!legacyDatabase.open(legacyPath, error)) {
        std::cerr << "FAIL: Could not open legacy database: " << error << '\n';
        return 1;
    }

    const char* legacySchema = R"SQL(
        CREATE TABLE accounts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL CHECK(length(trim(name)) > 0),
            type TEXT NOT NULL CHECK(length(trim(type)) > 0),
            initial_balance_paise INTEGER NOT NULL
        );
        CREATE TABLE budgets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category TEXT NOT NULL CHECK(length(trim(category)) > 0),
            month INTEGER NOT NULL CHECK(month BETWEEN 1 AND 12),
            year INTEGER NOT NULL CHECK(year > 0),
            limit_paise INTEGER NOT NULL CHECK(limit_paise > 0),
            UNIQUE(category COLLATE NOCASE, month, year)
        );
        INSERT INTO accounts (id, name, type, initial_balance_paise)
            VALUES (1, 'Legacy account', 'Cash', 100000);
        INSERT INTO budgets (id, category, month, year, limit_paise)
            VALUES (7, 'Food', 10, 2026, 50000);
    )SQL";

    if (!legacyDatabase.execute(legacySchema, error) ||
        !legacyDatabase.initializeSchema(error)) {
        std::cerr << "FAIL: Legacy database migration failed: " << error << '\n';
        return 1;
    }

    statement = nullptr;
    if (sqlite3_prepare_v2(
            legacyDatabase.handle(),
            "SELECT id, account_id, category, month, year, limit_paise "
            "FROM budgets WHERE id = 7;",
            -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "FAIL: Could not query migrated budget.\n";
        return 1;
    }

    const bool legacyBudgetPreserved =
        sqlite3_step(statement) == SQLITE_ROW &&
        sqlite3_column_int(statement, 0) == 7 &&
        sqlite3_column_type(statement, 1) == SQLITE_NULL &&
        std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))) == "Food" &&
        sqlite3_column_int(statement, 3) == 10 &&
        sqlite3_column_int(statement, 4) == 2026 &&
        sqlite3_column_int64(statement, 5) == 50000;
    sqlite3_finalize(statement);

    if (!legacyBudgetPreserved) {
        std::cerr << "FAIL: Migration did not preserve the old budget as unassigned.\n";
        return 1;
    }

    // Migration must be safe to run again, and the old unassigned budget
    // must coexist with account-specific budgets for the same period.
    if (!legacyDatabase.initializeSchema(error)) {
        std::cerr << "FAIL: Repeated schema initialization failed: " << error << '\n';
        return 1;
    }
    if (!legacyDatabase.execute(
            "INSERT INTO budgets (account_id, category, month, year, limit_paise) "
            "VALUES (1, 'food', 10, 2026, 75000);", error)) {
        std::cerr << "FAIL: Account-specific budget conflicts with legacy data: "
                  << error << '\n';
        return 1;
    }
    const bool sameAccountDuplicateAccepted = legacyDatabase.execute(
        "INSERT INTO budgets (account_id, category, month, year, limit_paise) "
        "VALUES (1, 'FOOD', 10, 2026, 90000);", error);
    if (sameAccountDuplicateAccepted) {
        std::cerr << "FAIL: Duplicate budget for the same account was accepted.\n";
        return 1;
    }

    legacyDatabase.close();
    std::remove(legacyPath.c_str());
    std::cout << "Test 7 passed: Legacy budgets migrated safely and remain unassigned.\n";

    std::cout << "\nAll DatabaseManager tests passed!\n";
    return 0;
}
