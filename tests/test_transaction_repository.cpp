#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Transaction.h"
#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"

#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const std::string path = "/tmp/pfbm_transaction_repository_test.db";
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

    // Transactions must reference an account that exists.
    AccountRepository accountRepository(database);
    Account account(0, "Test Account", "Checking", 100000);

    if (!accountRepository.add(account, error))
    {
        std::cerr << "FAIL: Create test account: " << error << '\n';
        return 1;
    }

    TransactionRepository repository(database);

    // Test 1: Create and save an expense.
    Transaction expense(
        0,
        account.getId(),
        12500,
        TransactionType::Expense,
        "Food",
        "Lunch",
        "2026-10-23"
    );

    if (!repository.add(expense, error))
    {
        std::cerr << "FAIL: Add transaction: " << error << '\n';
        return 1;
    }

    if (expense.getId() <= 0)
    {
        std::cerr << "FAIL: Transaction was not assigned a database ID.\n";
        return 1;
    }

    std::cout << "Test 1 passed: Expense created and saved.\n";

    // Test 2: Retrieve the transaction by ID.
    auto retrieved = repository.findById(expense.getId(), error);

    if (!retrieved ||
        retrieved->getAccountId() != account.getId() ||
        retrieved->getAmountPaise() != 12500 ||
        retrieved->getType() != TransactionType::Expense ||
        retrieved->getCategory() != "Food" ||
        retrieved->getDescription() != "Lunch" ||
        retrieved->getDate() != "2026-10-23")
    {
        std::cerr << "FAIL: Retrieved transaction does not match.\n";
        return 1;
    }

    std::cout << "Test 2 passed: Transaction retrieved correctly.\n";

    // Test 3: Save and retrieve an income transaction.
    Transaction income(
        0,
        account.getId(),
        250000,
        TransactionType::Income,
        "Salary",
        "October salary",
        "2026-10-01"
    );

    if (!repository.add(income, error))
    {
        std::cerr << "FAIL: Add income: " << error << '\n';
        return 1;
    }

    auto transactions = repository.getAll(error);

    if (!error.empty() || transactions.size() != 2)
    {
        std::cerr << "FAIL: Get all transactions: " << error << '\n';
        return 1;
    }

    bool foundIncome = false;

    for (const auto& transaction : transactions)
    {
        if (transaction.getId() == income.getId() &&
            transaction.getType() == TransactionType::Income)
        {
            foundIncome = true;
        }
    }

    if (!foundIncome)
    {
        std::cerr << "FAIL: Income transaction was not retrieved correctly.\n";
        return 1;
    }

    std::cout << "Test 3 passed: Income and all transactions retrieved.\n";

    // Test 4: Update the expense.
    Transaction updatedExpense(
        expense.getId(),
        account.getId(),
        15000,
        TransactionType::Expense,
        "Dining",
        "Dinner",
        "2026-10-22"
    );

    if (!repository.update(updatedExpense, error))
    {
        std::cerr << "FAIL: Update transaction: " << error << '\n';
        return 1;
    }

    retrieved = repository.findById(expense.getId(), error);

    if (!retrieved ||
        retrieved->getAmountPaise() != 15000 ||
        retrieved->getCategory() != "Dining" ||
        retrieved->getDescription() != "Dinner" ||
        retrieved->getDate() != "2026-10-22")
    {
        std::cerr << "FAIL: Updated transaction does not match.\n";
        return 1;
    }

    std::cout << "Test 4 passed: Transaction updated correctly.\n";

    // Test 5: Reject a transaction linked to a nonexistent account.
    Transaction invalidTransaction(
        0,
        999999,
        1000,
        TransactionType::Expense,
        "Other",
        "Invalid account transaction",
        "2026-10-23"
    );

    if (repository.add(invalidTransaction, error))
    {
        std::cerr << "FAIL: Transaction with nonexistent account was accepted.\n";
        return 1;
    }

    std::cout << "Test 5 passed: Invalid account reference rejected.\n";

    // Test 6: Delete the income transaction.
    if (!repository.remove(income.getId(), error))
    {
        std::cerr << "FAIL: Delete transaction: " << error << '\n';
        return 1;
    }

    retrieved = repository.findById(income.getId(), error);

    if (retrieved)
    {
        std::cerr << "FAIL: Deleted transaction still exists.\n";
        return 1;
    }

    std::cout << "Test 6 passed: Transaction deleted correctly.\n";

    // Test 7: Confirm transactions persist after reopening the database.
    database.close();

    if (!database.open(path, error))
    {
        std::cerr << "FAIL: Reopen database: " << error << '\n';
        return 1;
    }

    transactions = repository.getAll(error);

    if (!error.empty() ||
        transactions.size() != 1 ||
        transactions.front().getId() != expense.getId())
    {
        std::cerr << "FAIL: Persistent transaction data is incorrect: "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 7 passed: Transaction persisted after reopening.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll TransactionRepository tests passed!\n";

    return 0;
}
