#include "models/Account.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main()
{
    // Test 1: Create a valid account.
    Account account(0, " Savings Account ", " Bank ", 100000);

    assert(account.getId() == 0);
    assert(account.getName() == "Savings Account");
    assert(account.getType() == "Bank");
    assert(account.getInitialBalancePaise() == 100000);

    std::cout << "PASS: Valid account creation\n";

    // Test 2: Update the account ID.
    account.setId(1);
    assert(account.getId() == 1);

    std::cout << "PASS: Account ID update\n";

    // Test 3: An empty account name must be rejected.
    bool emptyNameRejected = false;

    try
    {
        account.setName("   ");
    }
    catch (const std::invalid_argument&)
    {
        emptyNameRejected = true;
    }

    assert(emptyNameRejected);
    std::cout << "PASS: Empty account name rejected\n";

    // Test 4: A negative account ID must be rejected.
    bool negativeIdRejected = false;

    try
    {
        account.setId(-1);
    }
    catch (const std::invalid_argument&)
    {
        negativeIdRejected = true;
    }

    assert(negativeIdRejected);
    std::cout << "PASS: Negative account ID rejected\n";

    // Test 5: An empty account type must be rejected.
    bool emptyTypeRejected = false;

    try
    {
        Account invalidAccount(0, "Cash", "   ", 0);
    }
    catch (const std::invalid_argument&)
    {
        emptyTypeRejected = true;
    }

    assert(emptyTypeRejected);
    std::cout << "PASS: Empty account type rejected\n";

    std::cout << "\nAll Account tests passed!\n";

    return 0;
}
