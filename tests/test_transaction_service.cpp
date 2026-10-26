#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Transaction.h"
#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"
#include "services/TransactionService.h"

#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const std::string path = "/tmp/pfbm_transaction_service_test.db";
    std::remove(path.c_str());

    DatabaseManager database;
    std::string error;

    if (!database.open(path, error))
    {
        std::cerr << "FAIL: Database open: " << error << '\n';
        return 1;
    }

    if (!database.initializeSchema(error))
    {
        std::cerr << "FAIL: Schema initialization: " << error << '\n';
        return 1;
    }

    AccountRepository accountRepository(database);
    TransactionRepository transactionRepository(database);

    Account account(0, "Main Account", "Checking", 1000000);

    if (!accountRepository.add(account, error))
    {
        std::cerr << "FAIL: Create account: " << error << '\n';
        return 1;
    }

    TransactionService service(
        accountRepository,
        transactionRepository
    );

    // Test 1: Add an expense.
    int expenseId = 0;

    if (!service.addExpense(
            account.getId(),
            15000,
            "Food",
            "Lunch",
            "2026-10-23",
            expenseId,
            error))
    {
        std::cerr << "FAIL: Add expense: " << error << '\n';
        return 1;
    }

    if (expenseId <= 0)
    {
        std::cerr << "FAIL: Expense ID was not assigned.\n";
        return 1;
    }

    std::cout << "Test 1 passed: Expense created successfully.\n";

    // Test 2: Add income.
    int incomeId = 0;

    if (!service.addIncome(
            account.getId(),
            500000,
            "Salary",
            "Monthly salary",
            "2026-10-01",
            incomeId,
            error))
    {
        std::cerr << "FAIL: Add income: " << error << '\n';
        return 1;
    }

    if (incomeId <= 0 || incomeId == expenseId)
    {
        std::cerr << "FAIL: Income ID is invalid.\n";
        return 1;
    }

    std::cout << "Test 2 passed: Income created successfully.\n";

    // Test 3: Retrieve transactions through the service.
    auto transactions = service.getAllTransactions(error);

    if (!error.empty() || transactions.size() != 2)
    {
        std::cerr << "FAIL: Retrieve transactions: " << error << '\n';
        return 1;
    }

    std::cout << "Test 3 passed: Transactions retrieved successfully.\n";

    // Test 4: Reject a nonexistent account.
    int invalidId = 0;

    if (service.addExpense(
            999999,
            10000,
            "Food",
            "Invalid account",
            "2026-10-23",
            invalidId,
            error))
    {
        std::cerr << "FAIL: Nonexistent account was accepted.\n";
        return 1;
    }

    if (invalidId != 0 || error.empty())
    {
        std::cerr << "FAIL: Invalid account error was not reported properly.\n";
        return 1;
    }

    std::cout << "Test 4 passed: Nonexistent account rejected.\n";

    // Test 5: Reject an invalid amount.
    error.clear();
    invalidId = 0;

    if (service.addExpense(
            account.getId(),
            0,
            "Food",
            "Zero amount",
            "2026-10-23",
            invalidId,
            error))
    {
        std::cerr << "FAIL: Zero amount was accepted.\n";
        return 1;
    }

    if (invalidId != 0 || error.empty())
    {
        std::cerr << "FAIL: Invalid amount error was not reported.\n";
        return 1;
    }

    std::cout << "Test 5 passed: Invalid amount rejected.\n";

    // Test 6: Reject an invalid date.
    error.clear();
    invalidId = 0;

    if (service.addExpense(
            account.getId(),
            10000,
            "Food",
            "Invalid date",
            "2026-02-30",
            invalidId,
            error))
    {
        std::cerr << "FAIL: Invalid date was accepted.\n";
        return 1;
    }

    if (invalidId != 0 || error.empty())
    {
        std::cerr << "FAIL: Invalid date error was not reported.\n";
        return 1;
    }

    std::cout << "Test 6 passed: Invalid date rejected.\n";

    // Test 7: Update an existing expense.
    Transaction updatedExpense(
        expenseId,
        account.getId(),
        25000,
        TransactionType::Expense,
        "Transport",
        "Bus fare",
        "2026-10-22"
    );

    if (!service.updateTransaction(updatedExpense, error))
    {
        std::cerr << "FAIL: Update transaction: " << error << '\n';
        return 1;
    }

    auto updated = transactionRepository.findById(expenseId, error);

    if (!updated ||
        updated->getAmountPaise() != 25000 ||
        updated->getCategory() != "Transport" ||
        updated->getDescription() != "Bus fare")
    {
        std::cerr << "FAIL: Updated transaction does not match.\n";
        return 1;
    }

    std::cout << "Test 7 passed: Transaction updated successfully.\n";

    // Test 8: Delete a transaction.
    if (!service.deleteTransaction(expenseId, error))
    {
        std::cerr << "FAIL: Delete transaction: " << error << '\n';
        return 1;
    }

    auto deleted = transactionRepository.findById(expenseId, error);

    if (deleted)
    {
        std::cerr << "FAIL: Deleted transaction still exists.\n";
        return 1;
    }

    std::cout << "Test 8 passed: Transaction deleted successfully.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll TransactionService tests passed!\n";

    return 0;
}
