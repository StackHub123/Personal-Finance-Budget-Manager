#include "database/DatabaseManager.h"
#include "repositories/AccountRepository.h"
#include "models/Account.h"

#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const std::string path = "/tmp/pfbm_account_repository_test.db";
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

    AccountRepository repository(database);

    // Test 1: Create and save an account.
    Account account(0, "Everyday Account", "Checking", 123456);

    if (!repository.add(account, error))
    {
        std::cerr << "FAIL: Add account: " << error << '\n';
        return 1;
    }

    if (account.getId() <= 0)
    {
        std::cerr << "FAIL: Account was not assigned a database ID.\n";
        return 1;
    }

    std::cout << "Test 1 passed: Account created and saved.\n";

    // Test 2: Retrieve the account by ID.
    auto retrieved = repository.findById(account.getId(), error);

    if (!retrieved)
    {
        std::cerr << "FAIL: Find account: " << error << '\n';
        return 1;
    }

    if (retrieved->getName() != "Everyday Account" ||
        retrieved->getType() != "Checking" ||
        retrieved->getInitialBalancePaise() != 123456)
    {
        std::cerr << "FAIL: Retrieved account data does not match.\n";
        return 1;
    }

    std::cout << "Test 2 passed: Account retrieved correctly.\n";

    // Test 3: Retrieve all accounts.
    Account secondAccount(0, "Cash Wallet", "Cash", 5000);

    if (!repository.add(secondAccount, error))
    {
        std::cerr << "FAIL: Add second account: " << error << '\n';
        return 1;
    }

    auto accounts = repository.getAll(error);

    if (!error.empty() || accounts.size() != 2)
    {
        std::cerr << "FAIL: Get all accounts: " << error << '\n';
        return 1;
    }

    std::cout << "Test 3 passed: All accounts retrieved.\n";

    // Test 4: Update an existing account.
    Account updatedAccount(
        account.getId(),
        "Emergency Savings",
        "Savings",
        987654
    );

    if (!repository.update(updatedAccount, error))
    {
        std::cerr << "FAIL: Update account: " << error << '\n';
        return 1;
    }

    retrieved = repository.findById(account.getId(), error);

    if (!retrieved ||
        retrieved->getName() != "Emergency Savings" ||
        retrieved->getType() != "Savings" ||
        retrieved->getInitialBalancePaise() != 987654)
    {
        std::cerr << "FAIL: Updated account data does not match.\n";
        return 1;
    }

    std::cout << "Test 4 passed: Account updated correctly.\n";

    // Test 5a: An account with transactions cannot be deleted.
    const std::string transactionSql =
        "INSERT INTO transactions "
        "(account_id, amount_paise, type, category, description, transaction_date) "
        "VALUES (" + std::to_string(account.getId()) +
        ", 10000, 'expense', 'Food', 'Lunch', '2026-10-23');";

    if (!database.execute(transactionSql, error))
    {
        std::cerr << "FAIL: Insert test transaction: " << error << '\n';
        return 1;
    }

    if (repository.remove(account.getId(), error))
    {
        std::cerr << "FAIL: Account with a transaction was deleted.\n";
        return 1;
    }

    std::cout << "Test 5a passed: Referenced account cannot be deleted.\n";

    // Test 5b: An account without transactions can be deleted.
    if (!repository.remove(secondAccount.getId(), error))
    {
        std::cerr << "FAIL: Delete account: " << error << '\n';
        return 1;
    }

    retrieved = repository.findById(secondAccount.getId(), error);

    if (retrieved)
    {
        std::cerr << "FAIL: Deleted account still exists.\n";
        return 1;
    }

    std::cout << "Test 5b passed: Unreferenced account deleted.\n";

    database.close();
    std::remove(path.c_str());

    std::cout << "\nAll AccountRepository tests passed!\n";

    return 0;
}
