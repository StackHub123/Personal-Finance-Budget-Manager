#include "models/Transaction.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main()
{
    // Test 1: Create a valid expense transaction.
    Transaction transaction(
        0, 1, 125050, TransactionType::Expense,
        " Groceries ", " Weekly shopping ", "2026-10-22"
    );

    assert(transaction.getId() == 0);
    assert(transaction.getAccountId() == 1);
    assert(transaction.getAmountPaise() == 125050);
    assert(transaction.getType() == TransactionType::Expense);
    assert(transaction.getCategory() == "Groceries");
    assert(transaction.getDescription() == "Weekly shopping");
    assert(transaction.getDate() == "2026-10-22");

    std::cout << "PASS: Valid transaction creation\n";

    // Test 2: Update the transaction ID.
    transaction.setId(1);
    assert(transaction.getId() == 1);

    std::cout << "PASS: Transaction ID update\n";

    // Test 3: Accept a valid income transaction.
    Transaction income(
        0, 1, 500000, TransactionType::Income,
        "Salary", "Monthly salary", "2024-02-29"
    );

    assert(income.getType() == TransactionType::Income);

    std::cout << "PASS: Income transaction and leap-year date\n";

    // Test 4: Reject a zero amount.
    bool zeroAmountRejected = false;

    try
    {
        transaction.setAmountPaise(0);
    }
    catch (const std::invalid_argument&)
    {
        zeroAmountRejected = true;
    }

    assert(zeroAmountRejected);
    std::cout << "PASS: Zero amount rejected\n";

    // Test 5: Reject a non-positive account ID.
    bool accountIdRejected = false;

    try
    {
        transaction.setAccountId(0);
    }
    catch (const std::invalid_argument&)
    {
        accountIdRejected = true;
    }

    assert(accountIdRejected);
    std::cout << "PASS: Invalid account ID rejected\n";

    // Test 6: Reject an empty category.
    bool categoryRejected = false;

    try
    {
        transaction.setCategory("   ");
    }
    catch (const std::invalid_argument&)
    {
        categoryRejected = true;
    }

    assert(categoryRejected);
    std::cout << "PASS: Empty category rejected\n";

    // Test 7: Reject an invalid calendar date.
    bool dateRejected = false;

    try
    {
        transaction.setDate("2025-02-29");
    }
    catch (const std::invalid_argument&)
    {
        dateRejected = true;
    }

    assert(dateRejected);
    std::cout << "PASS: Invalid date rejected\n";

    // Test 8: Reject an empty description.
    bool descriptionRejected = false;

    try
    {
        transaction.setDescription("   ");
    }
    catch (const std::invalid_argument&)
    {
        descriptionRejected = true;
    }

    assert(descriptionRejected);
    std::cout << "PASS: Empty description rejected\n";

    std::cout << "\nAll Transaction tests passed!\n";

    return 0;
}
