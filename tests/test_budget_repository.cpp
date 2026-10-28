#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Budget.h"
#include "repositories/AccountRepository.h"
#include "repositories/BudgetRepository.h"

#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const std::string path = "/tmp/pfbm_budget_repository_test.db";
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
    BudgetRepository repository(database);

    Account firstAccount(0, "First Account", "Cash", 0);
    Account secondAccount(0, "Second Account", "Savings", 0);
    if (!accountRepository.add(firstAccount, error) ||
        !accountRepository.add(secondAccount, error)) {
        std::cerr << "FAIL: Could not create test accounts: " << error << '\n';
        return 1;
    }

    // Test 1: Create and save a legacy/unassigned budget.
    Budget budget(0, "Food", 10, 2026, 250000);

    if (!repository.add(budget, error))
    {
        std::cerr << "FAIL: Add budget: " << error << '\n';
        return 1;
    }

    if (budget.getId() <= 0)
    {
        std::cerr << "FAIL: Budget was not assigned a database ID.\n";
        return 1;
    }

    std::cout << "Test 1 passed: Budget created and saved.\n";

    // Test 2: Retrieve the budget by ID.
    auto retrieved = repository.findById(budget.getId(), error);

    if (!retrieved ||
        retrieved->getCategory() != "Food" ||
        retrieved->getMonth() != 10 ||
        retrieved->getYear() != 2026 ||
        retrieved->getLimitPaise() != 250000)
    {
        std::cerr << "FAIL: Retrieved budget does not match.\n";
        return 1;
    }

    std::cout << "Test 2 passed: Budget retrieved correctly.\n";

    // Test 3: Save and retrieve multiple budgets.
    Budget secondBudget(0, "Transport", 10, 2026, 100000);

    if (!repository.add(secondBudget, error))
    {
        std::cerr << "FAIL: Add second budget: " << error << '\n';
        return 1;
    }

    auto budgets = repository.getAll(error);

    if (!error.empty() || budgets.size() != 2)
    {
        std::cerr << "FAIL: Get all budgets: " << error << '\n';
        return 1;
    }

    std::cout << "Test 3 passed: All budgets retrieved.\n";

    // Test 4: Identical category/month budgets are allowed for different accounts.
    Budget firstAccountFood(0, firstAccount.getId(), "Groceries", 10, 2026, 100000);
    Budget secondAccountFood(0, secondAccount.getId(), "groceries", 10, 2026, 200000);
    if (!repository.add(firstAccountFood, error) ||
        !repository.add(secondAccountFood, error)) {
        std::cerr << "FAIL: Same-period budgets for different accounts were rejected: "
                  << error << '\n';
        return 1;
    }

    auto firstAccountBudgets = repository.getByAccountId(firstAccount.getId(), error);
    if (!error.empty() || firstAccountBudgets.size() != 1 ||
        firstAccountBudgets.front().getAccountId() != firstAccount.getId()) {
        std::cerr << "FAIL: Account-filtered budget retrieval failed: " << error << '\n';
        return 1;
    }
    auto secondAccountBudgets = repository.getByAccountId(secondAccount.getId(), error);
    if (!error.empty() || secondAccountBudgets.size() != 1 ||
        secondAccountBudgets.front().getAccountId() != secondAccount.getId()) {
        std::cerr << "FAIL: Second account's budget retrieval failed: " << error << '\n';
        return 1;
    }

    Budget duplicateForFirst(0, firstAccount.getId(), "GROCERIES", 10, 2026, 300000);
    if (repository.add(duplicateForFirst, error)) {
        std::cerr << "FAIL: Duplicate budget for the same account was accepted.\n";
        return 1;
    }
    std::cout << "Test 4 passed: Budgets are scoped to their accounts.\n";

    // Test 5: Reject duplicate category/month/year unassigned budgets.
    // Category uniqueness is case-insensitive in the database.
    Budget duplicateBudget(0, "food", 10, 2026, 300000);

    if (repository.add(duplicateBudget, error))
    {
        std::cerr << "FAIL: Duplicate budget was accepted.\n";
        return 1;
    }

    std::cout << "Test 5 passed: Duplicate unassigned budget rejected.\n";

    // Test 6: Update an existing budget.
    Budget updatedBudget(
        budget.getId(),
        "Dining",
        11,
        2026,
        350000
    );

    if (!repository.update(updatedBudget, error))
    {
        std::cerr << "FAIL: Update budget: " << error << '\n';
        return 1;
    }

    retrieved = repository.findById(budget.getId(), error);

    if (!retrieved ||
        retrieved->getCategory() != "Dining" ||
        retrieved->getMonth() != 11 ||
        retrieved->getYear() != 2026 ||
        retrieved->getLimitPaise() != 350000)
    {
        std::cerr << "FAIL: Updated budget does not match.\n";
        return 1;
    }

    std::cout << "Test 6 passed: Budget updated correctly.\n";

    // Test 7: Delete a budget.
    if (!repository.remove(secondBudget.getId(), error))
    {
        std::cerr << "FAIL: Delete budget: " << error << '\n';
        return 1;
    }

    retrieved = repository.findById(secondBudget.getId(), error);

    if (retrieved)
    {
        std::cerr << "FAIL: Deleted budget still exists.\n";
        return 1;
    }

    std::cout << "Test 7 passed: Budget deleted correctly.\n";

    // Test 8: Verify persistence after reopening the database.
    database.close();

    if (!database.open(path, error))
    {
        std::cerr << "FAIL: Reopen database: " << error << '\n';
        return 1;
    }

    budgets = repository.getAll(error);

    if (!error.empty() ||
        budgets.size() != 3)
    {
        std::cerr << "FAIL: Persisted budget data is incorrect: "
                  << error << '\n';
        return 1;
    }

    bool updatedUnassignedFound = false;
    bool firstAccountFound = false;
    bool secondAccountFound = false;
    for (const auto &persisted : budgets) {
        if (persisted.getId() == budget.getId() &&
            persisted.getCategory() == "Dining" &&
            persisted.getLimitPaise() == 350000 &&
            persisted.getAccountId() == 0)
            updatedUnassignedFound = true;
        if (persisted.getId() == firstAccountFood.getId() &&
            persisted.getAccountId() == firstAccount.getId())
            firstAccountFound = true;
        if (persisted.getId() == secondAccountFood.getId() &&
            persisted.getAccountId() == secondAccount.getId())
            secondAccountFound = true;
    }
    if (!updatedUnassignedFound || !firstAccountFound || !secondAccountFound) {
        std::cerr << "FAIL: Persisted budgets lost their account associations.\n";
        return 1;
    }

    std::cout << "Test 8 passed: Budgets and account associations persist after reopening.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll BudgetRepository tests passed!\n";

    return 0;
}
