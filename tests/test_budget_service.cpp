#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Budget.h"
#include "models/Transaction.h"
#include "repositories/AccountRepository.h"
#include "repositories/BudgetRepository.h"
#include "repositories/TransactionRepository.h"
#include "services/BudgetService.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::string path = "/tmp/pfbm_budget_service_test.db";
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
    BudgetRepository budgetRepository(database);

    // Create an account because transactions must reference a real account.
    Account account(0, "Test Account", "Checking", 100000);

    if (!accountRepository.add(account, error))
    {
        std::cerr << "FAIL: Create account: " << error << '\n';
        return 1;
    }

    Account secondAccount(0, "Second Test Account", "Savings", 0);
    if (!accountRepository.add(secondAccount, error)) {
        std::cerr << "FAIL: Create second account: " << error << '\n';
        return 1;
    }

    // Add transactions directly through the repository to set up test data.
    auto addTransaction = [&](std::int64_t amount,
                              TransactionType type,
                              const std::string& category,
                              const std::string& date,
                              const std::string& description) -> bool
    {
        Transaction transaction(
            0,
            account.getId(),
            amount,
            type,
            category,
            description,
            date
        );

        error.clear();

        if (!transactionRepository.add(transaction, error))
        {
            std::cerr << "FAIL: Add test transaction: "
                      << error << '\n';
            return false;
        }

        return true;
    };

    // October Food expenses: ₹1,200 + ₹850 = ₹2,050.
    if (!addTransaction(
            120000, TransactionType::Expense,
            "Food", "2026-10-05", "Groceries") ||
        !addTransaction(
            85000, TransactionType::Expense,
            "Food", "2026-10-20", "Dinner"))
    {
        return 1;
    }

    // These transactions must not count toward October's Food budget.
    if (!addTransaction(
            25000, TransactionType::Expense,
            "Food", "2026-09-30", "September groceries") ||
        !addTransaction(
            50000, TransactionType::Expense,
            "Transport", "2026-10-15", "Bus and taxi") ||
        !addTransaction(
            999000, TransactionType::Income,
            "Food", "2026-10-01", "Income test"))
    {
        return 1;
    }

    // The same category/month in another account must not affect this account's budget.
    Transaction secondAccountFood(
        0, secondAccount.getId(), 30000, TransactionType::Expense,
        "food", "Second account groceries", "2026-10-12");
    if (!transactionRepository.add(secondAccountFood, error)) {
        std::cerr << "FAIL: Add second-account transaction: " << error << '\n';
        return 1;
    }

    BudgetService service(budgetRepository, transactionRepository);

    // Test 1: Create a monthly budget.
    int foodBudgetId = 0;
    error.clear();

    if (!service.addBudget(
            account.getId(), "Food", 10, 2026, 250000, foodBudgetId, error) ||
        foodBudgetId <= 0)
    {
        std::cerr << "FAIL: Create Food budget: "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 1 passed: Budget created.\n";

    // Test 2: Calculate spending and remaining budget correctly.
    error.clear();
    auto foodStatus = service.getBudgetStatus(foodBudgetId, error);

    if (!foodStatus ||
        !error.empty() ||
        foodStatus->spentPaise != 205000 ||
        foodStatus->remainingPaise != 45000 ||
        foodStatus->overBudget)
    {
        std::cerr << "FAIL: Food budget calculation is incorrect. "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 2 passed: Spending and remaining amount "
                 "calculated correctly.\n";

    int secondFoodBudgetId = 0;
    if (!service.addBudget(
            secondAccount.getId(), "Food", 10, 2026, 100000,
            secondFoodBudgetId, error) || secondFoodBudgetId <= 0) {
        std::cerr << "FAIL: Same category budget for second account: "
                  << error << '\n';
        return 1;
    }
    auto secondFoodStatus = service.getBudgetStatus(secondFoodBudgetId, error);
    if (!secondFoodStatus || !error.empty() ||
        secondFoodStatus->spentPaise != 30000 ||
        secondFoodStatus->remainingPaise != 70000 ||
        secondFoodStatus->budget.getAccountId() != secondAccount.getId()) {
        std::cerr << "FAIL: Second account budget included another account's spending. "
                  << error << '\n';
        return 1;
    }
    std::cout << "Test 3 passed: Same-category spending is isolated by account.\n";

    // Test 4: Detect when spending exceeds the budget.
    int transportBudgetId = 0;
    error.clear();

    if (!service.addBudget(
            account.getId(), "Transport", 10, 2026, 40000,
            transportBudgetId, error))
    {
        std::cerr << "FAIL: Create Transport budget: "
                  << error << '\n';
        return 1;
    }

    error.clear();
    auto transportStatus =
        service.getBudgetStatus(transportBudgetId, error);

    if (!transportStatus ||
        !error.empty() ||
        transportStatus->spentPaise != 50000 ||
        transportStatus->remainingPaise != -10000 ||
        !transportStatus->overBudget)
    {
        std::cerr << "FAIL: Over-budget detection is incorrect. "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 4 passed: Over-budget situation detected.\n";

    // Test 4: Retrieve the statuses of all budgets.
    error.clear();
    auto statuses = service.getAllBudgetStatuses(error);

    if (!error.empty() || statuses.size() != 3)
    {
        std::cerr << "FAIL: Retrieve all budget statuses: "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 5 passed: All budget statuses retrieved.\n";

    // Test 5: Update a budget and recalculate its status.
    Budget updatedFoodBudget(
        foodBudgetId, account.getId(), "Food", 10, 2026, 200000
    );

    error.clear();

    if (!service.updateBudget(updatedFoodBudget, error))
    {
        std::cerr << "FAIL: Update Food budget: "
                  << error << '\n';
        return 1;
    }

    error.clear();
    foodStatus = service.getBudgetStatus(foodBudgetId, error);

    if (!foodStatus ||
        !error.empty() ||
        foodStatus->spentPaise != 205000 ||
        foodStatus->remainingPaise != -5000 ||
        !foodStatus->overBudget)
    {
        std::cerr << "FAIL: Updated budget status is incorrect. "
                  << error << '\n';
        return 1;
    }

    std::cout << "Test 6 passed: Updated budget status is correct.\n";

    // Test 6: Reject an invalid budget ID.
    error.clear();

    if (service.getBudgetStatus(0, error) || error.empty())
    {
        std::cerr << "FAIL: Invalid budget ID was not rejected.\n";
        return 1;
    }

    std::cout << "Test 7 passed: Invalid budget ID rejected.\n";

    // Test 7: Handle a budget ID that does not exist.
    error.clear();

    if (service.getBudgetStatus(999999, error) || error.empty())
    {
        std::cerr << "FAIL: Nonexistent budget was not handled.\n";
        return 1;
    }

    std::cout << "Test 8 passed: Nonexistent budget handled.\n";

    // Test 8: Delete a budget.
    error.clear();

    if (!service.deleteBudget(transportBudgetId, error))
    {
        std::cerr << "FAIL: Delete Transport budget: "
                  << error << '\n';
        return 1;
    }

    error.clear();
    statuses = service.getAllBudgetStatuses(error);

    if (!error.empty() || statuses.size() != 2)
    {
        std::cerr << "FAIL: Budget deletion was unsuccessful. "
                  << error << '\n';
        return 1;
    }

    bool firstFoodBudgetRemains = false;
    bool secondFoodBudgetRemains = false;
    for (const auto &status : statuses) {
        if (status.budget.getId() == foodBudgetId)
            firstFoodBudgetRemains = true;
        if (status.budget.getId() == secondFoodBudgetId)
            secondFoodBudgetRemains = true;
    }
    if (!firstFoodBudgetRemains || !secondFoodBudgetRemains) {
        std::cerr << "FAIL: Deleting one budget affected another account's budget.\n";
        return 1;
    }
    std::cout << "Test 9 passed: Budget deletion is isolated to its ID.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll BudgetService tests passed!\n";

    return 0;
}
