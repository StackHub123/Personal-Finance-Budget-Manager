#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Transaction.h"
#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"
#include "services/FinanceService.h"

#include <cstdio>
#include <iostream>
#include <optional>
#include <string>

int main()
{
    const std::string path = "/tmp/pfbm_finance_service_test.db";
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

    // Create two accounts.
    Account mainAccount(0, "Main Account", "Checking", 1000000);
    Account secondAccount(0, "Second Account", "Savings", 0);

    if (!accountRepository.add(mainAccount, error) ||
        !accountRepository.add(secondAccount, error))
    {
        std::cerr << "FAIL: Create accounts: " << error << '\n';
        return 1;
    }

    // Transactions belonging to the main account.
    Transaction salary(
        0, mainAccount.getId(), 500000,
        TransactionType::Income, "Salary",
        "October salary", "2026-10-01"
    );

    Transaction lunch(
        0, mainAccount.getId(), 200000,
        TransactionType::Expense, "Food",
        "Lunch", "2026-10-15"
    );

    Transaction groceries(
        0, mainAccount.getId(), 30000,
        TransactionType::Expense, "Food",
        "Groceries", "2026-10-31"
    );

    // This income is outside the October reporting period.
    Transaction septemberIncome(
        0, mainAccount.getId(), 100000,
        TransactionType::Income, "Other",
        "September income", "2026-09-30"
    );

    // This transaction belongs to another account.
    Transaction secondAccountIncome(
        0, secondAccount.getId(), 900000,
        TransactionType::Income, "Income",
        "Second account income", "2026-10-10"
    );

    if (!transactionRepository.add(salary, error) ||
        !transactionRepository.add(lunch, error) ||
        !transactionRepository.add(groceries, error) ||
        !transactionRepository.add(septemberIncome, error) ||
        !transactionRepository.add(secondAccountIncome, error))
    {
        std::cerr << "FAIL: Create transactions: " << error << '\n';
        return 1;
    }

    FinanceService service(accountRepository, transactionRepository);

    // Test 1: Current account balance.
    // 1,000,000 + 500,000 + 100,000 - 200,000 - 30,000
    // = 1,370,000 paise.
    auto balance =
        service.calculateAccountBalancePaise(mainAccount.getId(), error);

    if (!balance || *balance != 1370000)
    {
        std::cerr << "FAIL: Account balance: " << error << '\n';
        return 1;
    }

    std::cout << "Test 1 passed: Account balance calculated correctly.\n";

    // Test 2: October income, across all accounts.
    // 500,000 + 900,000 = 1,400,000 paise.
    auto income = service.calculateTotalIncomePaise(
        "2026-10-01", "2026-10-31", error);

    if (!income || *income != 1400000)
    {
        std::cerr << "FAIL: Total income: " << error << '\n';
        return 1;
    }

    std::cout << "Test 2 passed: Total income calculated correctly.\n";

    // Test 3: October expenses.
    // 200,000 + 30,000 = 230,000 paise.
    auto expenses = service.calculateTotalExpensesPaise(
        "2026-10-01", "2026-10-31", error);

    if (!expenses || *expenses != 230000)
    {
        std::cerr << "FAIL: Total expenses: " << error << '\n';
        return 1;
    }

    std::cout << "Test 3 passed: Total expenses calculated correctly.\n";

    // Test 4: Net savings across all accounts for October.
    // 1,400,000 - 230,000 = 1,170,000 paise.
    auto savings = service.calculateNetSavingsPaise(
        "2026-10-01", "2026-10-31", error);

    if (!savings || *savings != 1170000)
    {
        std::cerr << "FAIL: Net savings: " << error << '\n';
        return 1;
    }

    std::cout << "Test 4 passed: Net savings calculated correctly.\n";

    // Test 5: Both boundary dates are included.
    auto boundaryExpenses = service.calculateTotalExpensesPaise(
        "2026-10-01", "2026-10-31", error);

    if (!boundaryExpenses || *boundaryExpenses != 230000)
    {
        std::cerr << "FAIL: Date boundary handling: " << error << '\n';
        return 1;
    }

    std::cout << "Test 5 passed: Date-range boundaries handled correctly.\n";

    // Test 6a: Reject an impossible date.
    auto invalidDate = service.calculateTotalIncomePaise(
        "2026-02-30", "2026-03-31", error);

    if (invalidDate.has_value() || error.empty())
    {
        std::cerr << "FAIL: Invalid date was not rejected.\n";
        return 1;
    }

    // Test 6b: Reject a reversed date range.
    auto reversedRange = service.calculateTotalExpensesPaise(
        "2026-10-31", "2026-10-01", error);

    if (reversedRange.has_value() || error.empty())
    {
        std::cerr << "FAIL: Reversed date range was not rejected.\n";
        return 1;
    }

    std::cout << "Test 6 passed: Invalid dates and ranges rejected.\n";

    // Test 7: Handle a nonexistent account.
    auto missingAccount =
        service.calculateAccountBalancePaise(999999, error);

    if (missingAccount.has_value() || error.empty())
    {
        std::cerr << "FAIL: Nonexistent account was not handled.\n";
        return 1;
    }

    std::cout << "Test 7 passed: Nonexistent account handled correctly.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll FinanceService tests passed!\n";

    return 0;
}
