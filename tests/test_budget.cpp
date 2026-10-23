#include "models/Budget.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

template <typename Function>
void expectInvalidArgument(Function function)
{
    bool exceptionThrown = false;

    try
    {
        function();
    }
    catch (const std::invalid_argument&)
    {
        exceptionThrown = true;
    }

    assert(exceptionThrown);
}

int main()
{
    // Test 1: Create a valid budget.
    Budget budget(0, "  Food  ", 10, 2026, 250000);

    assert(budget.getId() == 0);
    assert(budget.getCategory() == "Food");
    assert(budget.getMonth() == 10);
    assert(budget.getYear() == 2026);
    assert(budget.getLimitPaise() == 250000);

    std::cout << "Test 1 passed: Valid budget creation.\n";

    // Test 2: Test setters.
    budget.setId(1);
    budget.setCategory(" Transport ");
    budget.setMonth(11);
    budget.setYear(2027);
    budget.setLimitPaise(500000);

    assert(budget.getId() == 1);
    assert(budget.getCategory() == "Transport");
    assert(budget.getMonth() == 11);
    assert(budget.getYear() == 2027);
    assert(budget.getLimitPaise() == 500000);

    std::cout << "Test 2 passed: Setters and whitespace trimming.\n";

    // Test 3: Reject invalid constructor arguments.
    expectInvalidArgument([] { Budget(-1, "Food", 10, 2026, 10000); });
    expectInvalidArgument([] { Budget(0, "   ", 10, 2026, 10000); });
    expectInvalidArgument([] { Budget(0, "Food", 0, 2026, 10000); });
    expectInvalidArgument([] { Budget(0, "Food", 13, 2026, 10000); });
    expectInvalidArgument([] { Budget(0, "Food", 10, 0, 10000); });
    expectInvalidArgument([] { Budget(0, "Food", 10, 2026, 0); });
    expectInvalidArgument([] { Budget(0, "Food", 10, 2026, -10000); });

    std::cout << "Test 3 passed: Invalid constructor arguments rejected.\n";

    // Test 4: Reject invalid setter arguments.
    expectInvalidArgument([&] { budget.setId(-1); });
    expectInvalidArgument([&] { budget.setCategory("   "); });
    expectInvalidArgument([&] { budget.setMonth(0); });
    expectInvalidArgument([&] { budget.setMonth(13); });
    expectInvalidArgument([&] { budget.setYear(0); });
    expectInvalidArgument([&] { budget.setLimitPaise(0); });
    expectInvalidArgument([&] { budget.setLimitPaise(-1); });

    std::cout << "Test 4 passed: Invalid setter arguments rejected.\n";

    std::cout << "\nAll Budget tests passed!\n";

    return 0;
}
