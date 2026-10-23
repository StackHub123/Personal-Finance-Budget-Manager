#include "database/DatabaseManager.h"
#include "models/Budget.h"
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

    BudgetRepository repository(database);

    // Test 1: Create and save a budget.
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

    // Test 4: Reject duplicate category/month/year budgets.
    // Category uniqueness is case-insensitive in the database.
    Budget duplicateBudget(0, "food", 10, 2026, 300000);

    if (repository.add(duplicateBudget, error))
    {
        std::cerr << "FAIL: Duplicate budget was accepted.\n";
        return 1;
    }

    std::cout << "Test 4 passed: Duplicate budget rejected.\n";

    // Test 5: Update an existing budget.
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

    std::cout << "Test 5 passed: Budget updated correctly.\n";

    // Test 6: Delete a budget.
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

    std::cout << "Test 6 passed: Budget deleted correctly.\n";

    // Test 7: Verify persistence after reopening the database.
    database.close();

    if (!database.open(path, error))
    {
        std::cerr << "FAIL: Reopen database: " << error << '\n';
        return 1;
    }

    budgets = repository.getAll(error);

    if (!error.empty() ||
        budgets.size() != 1 ||
        budgets.front().getId() != budget.getId() ||
        budgets.front().getCategory() != "Dining" ||
        budgets.front().getLimitPaise() != 350000)
    {
        std::cerr << "FAIL: Persisted budget data is incorrect: "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 7 passed: Budget persisted after reopening.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll BudgetRepository tests passed!\n";

    return 0;
}
