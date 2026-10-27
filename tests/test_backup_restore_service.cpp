#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Budget.h"
#include "models/Transaction.h"
#include "repositories/AccountRepository.h"
#include "repositories/BudgetRepository.h"
#include "repositories/TransactionRepository.h"
#include "services/BackupRestoreService.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void verifyRestoredData(
    AccountRepository& accountRepository,
    TransactionRepository& transactionRepository,
    BudgetRepository& budgetRepository)
{
    std::string error;

    const auto accounts = accountRepository.getAll(error);
    require(error.empty(), "Could not retrieve accounts: " + error);

    error.clear();
    const auto transactions = transactionRepository.getAll(error);
    require(error.empty(), "Could not retrieve transactions: " + error);

    error.clear();
    const auto budgets = budgetRepository.getAll(error);
    require(error.empty(), "Could not retrieve budgets: " + error);

    require(accounts.size() == 1, "Expected exactly one restored account.");
    require(transactions.size() == 1,
            "Expected exactly one restored transaction.");
    require(budgets.size() == 1, "Expected exactly one restored budget.");

    require(accounts[0].getId() == 101,
            "The restored account ID was not preserved.");
    require(accounts[0].getName() == "Restored account",
            "The restored account name is incorrect.");

    require(transactions[0].getId() == 201,
            "The restored transaction ID was not preserved.");
    require(transactions[0].getAccountId() == 101,
            "The restored transaction references the wrong account.");
    require(transactions[0].getAmountPaise() == 12345,
            "The restored transaction amount is incorrect.");

    require(budgets[0].getId() == 301,
            "The restored budget ID was not preserved.");
    require(budgets[0].getCategory() == "Travel",
            "The restored budget category is incorrect.");
    require(budgets[0].getLimitPaise() == 80000,
            "The restored budget limit is incorrect.");
}

int main()
{
    try {
        // Use a separate in-memory database. The real application database
        // is not opened or modified by this test.
        DatabaseManager database;
        std::string error;

        require(database.open(":memory:", error),
                "Could not open test database: " + error);

        require(database.initializeSchema(error),
                "Could not initialize test schema: " + error);

        AccountRepository accountRepository(database);
        TransactionRepository transactionRepository(database);
        BudgetRepository budgetRepository(database);

        BackupRestoreService restoreService(
            database,
            accountRepository,
            transactionRepository,
            budgetRepository);

        // Seed the database with records that should be replaced.
        Account oldAccount(0, "Old account", "Cash", 50000);
        require(accountRepository.add(oldAccount, error),
                "Could not create initial account: " + error);

        Transaction oldTransaction(
            0, oldAccount.getId(), 1500,
            TransactionType::Expense,
            "Food", "Old transaction", "2026-10-01");

        error.clear();
        require(transactionRepository.add(oldTransaction, error),
                "Could not create initial transaction: " + error);

        Budget oldBudget(0, "Food", 10, 2026, 20000);

        error.clear();
        require(budgetRepository.add(oldBudget, error),
                "Could not create initial budget: " + error);

        // These represent records read from a valid backup.
        const std::vector<Account> restoredAccounts = {
            Account(101, "Restored account", "Savings", 250000)
        };

        const std::vector<Transaction> restoredTransactions = {
            Transaction(
                201, 101, 12345,
                TransactionType::Income,
                "Salary", "Restored transaction", "2026-10-05")
        };

        const std::vector<Budget> restoredBudgets = {
            Budget(301, "Travel", 10, 2026, 80000)
        };

        // Test 1: a successful restore replaces all records.
        error.clear();
        require(
            restoreService.replaceAll(
                restoredAccounts,
                restoredTransactions,
                restoredBudgets,
                error),
            "Restore unexpectedly failed: " + error);

        verifyRestoredData(
            accountRepository,
            transactionRepository,
            budgetRepository);

        std::cout
            << "Test 1 passed: restore replaced records and preserved IDs "
               "and account relationships.\n";

        // Test 2: duplicate budget periods force a database constraint
        // failure after the restore has already started inserting records.
        // The service should roll the entire operation back.
        const std::vector<Account> failedAccounts = {
            Account(102, "Temporary account", "Cash", 10000)
        };

        const std::vector<Transaction> failedTransactions = {
            Transaction(
                202, 102, 500,
                TransactionType::Expense,
                "Test", "Transaction before failure", "2026-10-06")
        };

        const std::vector<Budget> duplicateBudgets = {
            Budget(302, "Duplicate", 11, 2026, 10000),
            Budget(303, "Duplicate", 11, 2026, 15000)
        };

        error.clear();
        const bool failedRestoreSucceeded =
            restoreService.replaceAll(
                failedAccounts,
                failedTransactions,
                duplicateBudgets,
                error);

        require(!failedRestoreSucceeded,
                "Restore should have failed for duplicate budget periods.");

        verifyRestoredData(
            accountRepository,
            transactionRepository,
            budgetRepository);

        std::cout
            << "Test 2 passed: failed restore rolled back; previously "
               "restored records remain intact.\n";

        // Test 3: a transaction referencing an absent account must be
        // rejected before the database is changed.
        const std::vector<Account> invalidAccounts = {
            Account(103, "Another account", "Cash", 5000)
        };

        const std::vector<Transaction> invalidTransactions = {
            Transaction(
                203, 999, 1000,
                TransactionType::Expense,
                "Test", "Missing account reference", "2026-10-07")
        };

        error.clear();
        const bool invalidRestoreSucceeded =
            restoreService.replaceAll(
                invalidAccounts,
                invalidTransactions,
                {},
                error);

        require(!invalidRestoreSucceeded,
                "Restore should reject a missing account reference.");

        verifyRestoredData(
            accountRepository,
            transactionRepository,
            budgetRepository);

        std::cout
            << "Test 3 passed: invalid account reference was rejected "
               "without changing existing records.\n";

        std::cout << "\nAll BackupRestoreService tests passed!\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "TEST FAILED: " << exception.what() << '\n';
        return 1;
    }
}
