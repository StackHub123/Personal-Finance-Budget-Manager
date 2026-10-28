#include "services/FinanceService.h"

#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

namespace
{
    bool isLeapYear(int year)
    {
        return year % 400 == 0 ||
               (year % 4 == 0 && year % 100 != 0);
    }

    bool isValidDate(const std::string& date)
    {
        if (date.size() != 10 ||
            date[4] != '-' ||
            date[7] != '-')
        {
            return false;
        }

        for (std::size_t i = 0; i < date.size(); ++i)
        {
            if (i == 4 || i == 7)
            {
                continue;
            }

            if (!std::isdigit(static_cast<unsigned char>(date[i])))
            {
                return false;
            }
        }

        const int year = std::stoi(date.substr(0, 4));
        const int month = std::stoi(date.substr(5, 2));
        const int day = std::stoi(date.substr(8, 2));

        if (year < 1 || month < 1 || month > 12)
        {
            return false;
        }

        const int daysInMonth[] = {
            31, 28, 31, 30, 31, 30,
            31, 31, 30, 31, 30, 31
        };

        int maximumDay = daysInMonth[month - 1];

        if (month == 2 && isLeapYear(year))
        {
            maximumDay = 29;
        }

        return day >= 1 && day <= maximumDay;
    }

    bool validateDateRange(
        const std::string& startDate,
        const std::string& endDate,
        std::string& error)
    {
        if (!isValidDate(startDate) || !isValidDate(endDate))
        {
            error = "Dates must be valid calendar dates in YYYY-MM-DD format.";
            return false;
        }

        // ISO-format dates can be compared lexicographically.
        if (startDate > endDate)
        {
            error = "Start date cannot be later than end date.";
            return false;
        }

        return true;
    }

    bool checkedAdd(
        std::int64_t current,
        std::int64_t amount,
        std::int64_t& result)
    {
        if (amount > 0 &&
            current > std::numeric_limits<std::int64_t>::max() - amount)
        {
            return false;
        }

        if (amount < 0 &&
            current < std::numeric_limits<std::int64_t>::min() - amount)
        {
            return false;
        }

        result = current + amount;
        return true;
    }

    bool checkedSubtract(
        std::int64_t current,
        std::int64_t amount,
        std::int64_t& result)
    {
        if (amount > 0 &&
            current < std::numeric_limits<std::int64_t>::min() + amount)
        {
            return false;
        }

        if (amount < 0 &&
            current > std::numeric_limits<std::int64_t>::max() + amount)
        {
            return false;
        }

        result = current - amount;
        return true;
    }

    bool calculateTransactionTotal(
        const TransactionRepository& repository,
        TransactionType desiredType,
        const std::string& startDate,
        const std::string& endDate,
        std::int64_t& total,
        std::string& error)
    {
        if (!validateDateRange(startDate, endDate, error))
        {
            return false;
        }

        const auto transactions = repository.getAll(error);

        if (!error.empty())
        {
            return false;
        }

        total = 0;

        for (const auto& transaction : transactions)
        {
            if (transaction.getType() != desiredType)
            {
                continue;
            }

            // Both boundary dates are included.
            if (transaction.getDate() < startDate ||
                transaction.getDate() > endDate)
            {
                continue;
            }

            std::int64_t newTotal = 0;

            if (!checkedAdd(
                    total,
                    transaction.getAmountPaise(),
                    newTotal))
            {
                error = "Financial total exceeds the supported integer range.";
                return false;
            }

            total = newTotal;
        }

        return true;
    }
}

FinanceService::FinanceService(
    AccountRepository& accountRepository,
    TransactionRepository& transactionRepository)
    : m_accountRepository(accountRepository),
      m_transactionRepository(transactionRepository)
{
}

std::optional<std::int64_t>
FinanceService::calculateAccountBalancePaise(
    int accountId,
    std::string& error) const
{
    const auto account = m_accountRepository.findById(accountId, error);

    if (!account)
    {
        if (error.empty())
        {
            error = "Account could not be found.";
        }

        return std::nullopt;
    }

    const auto transactions = m_transactionRepository.getAll(error);

    if (!error.empty())
    {
        return std::nullopt;
    }

    std::int64_t balance = account->getInitialBalancePaise();

    for (const auto& transaction : transactions)
    {
        if (transaction.getAccountId() != accountId)
        {
            continue;
        }

        std::int64_t newBalance = 0;
        bool succeeded = false;

        if (transaction.getType() == TransactionType::Income)
        {
            succeeded = checkedAdd(
                balance,
                transaction.getAmountPaise(),
                newBalance
            );
        }
        else if (transaction.getType() == TransactionType::Expense)
        {
            succeeded = checkedSubtract(
                balance,
                transaction.getAmountPaise(),
                newBalance
            );
        }
        else
        {
            error = "Encountered an invalid transaction type.";
            return std::nullopt;
        }

        if (!succeeded)
        {
            error = "Account balance exceeds the supported integer range.";
            return std::nullopt;
        }

        balance = newBalance;
    }

    error.clear();
    return balance;
}

std::optional<std::int64_t>
FinanceService::calculateTotalIncomePaise(
    const std::string& startDate,
    const std::string& endDate,
    std::string& error) const
{
    std::int64_t total = 0;

    if (!calculateTransactionTotal(
            m_transactionRepository,
            TransactionType::Income,
            startDate,
            endDate,
            total,
            error))
    {
        return std::nullopt;
    }

    error.clear();
    return total;
}

std::optional<std::int64_t>
FinanceService::calculateTotalExpensesPaise(
    const std::string& startDate,
    const std::string& endDate,
    std::string& error) const
{
    std::int64_t total = 0;

    if (!calculateTransactionTotal(
            m_transactionRepository,
            TransactionType::Expense,
            startDate,
            endDate,
            total,
            error))
    {
        return std::nullopt;
    }

    error.clear();
    return total;
}

std::optional<std::int64_t>
FinanceService::calculateNetSavingsPaise(
    const std::string& startDate,
    const std::string& endDate,
    std::string& error) const
{
    const auto income = calculateTotalIncomePaise(
        startDate, endDate, error);

    if (!income)
    {
        return std::nullopt;
    }

    const auto expenses = calculateTotalExpensesPaise(
        startDate, endDate, error);

    if (!expenses)
    {
        return std::nullopt;
    }

    std::int64_t savings = 0;

    if (!checkedSubtract(*income, *expenses, savings))
    {
        error = "Net savings exceeds the supported integer range.";
        return std::nullopt;
    }

    error.clear();
    return savings;
}
