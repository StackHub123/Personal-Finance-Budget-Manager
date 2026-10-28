#include "models/Transaction.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace
{
    std::string trim(const std::string& value)
    {
        const auto first = std::find_if_not(
            value.begin(), value.end(),
            [](unsigned char ch) {
                return std::isspace(ch) != 0;
            });

        const auto last = std::find_if_not(
            value.rbegin(), value.rend(),
            [](unsigned char ch) {
                return std::isspace(ch) != 0;
            }).base();

        if (first >= last)
            return {};

        return std::string(first, last);
    }

    bool isValidDate(const std::string& date)
    {
        if (date.size() != 10 ||
            date[4] != '-' || date[7] != '-')
            return false;

        for (std::size_t i = 0; i < date.size(); ++i)
        {
            if (i != 4 && i != 7 &&
                !std::isdigit(static_cast<unsigned char>(date[i])))
                return false;
        }

        const int year = std::stoi(date.substr(0, 4));
        const int month = std::stoi(date.substr(5, 2));
        const int day = std::stoi(date.substr(8, 2));

        if (year < 1 || month < 1 || month > 12)
            return false;

        const int daysInMonth[] = {
            31, 28, 31, 30, 31, 30,
            31, 31, 30, 31, 30, 31
        };

        int maxDay = daysInMonth[month - 1];

        const bool leapYear =
            (year % 400 == 0) ||
            (year % 4 == 0 && year % 100 != 0);

        if (month == 2 && leapYear)
            maxDay = 29;

        return day >= 1 && day <= maxDay;
    }

    void validateType(TransactionType type)
    {
        if (type != TransactionType::Income &&
            type != TransactionType::Expense)
        {
            throw std::invalid_argument("Invalid transaction type.");
        }
    }
}

Transaction::Transaction(int id,
                         int accountId,
                         std::int64_t amountPaise,
                         TransactionType type,
                         std::string category,
                         std::string description,
                         std::string date)
    : m_id(id),
      m_accountId(accountId),
      m_amountPaise(amountPaise),
      m_type(type)
{
    if (id < 0)
        throw std::invalid_argument("Transaction ID cannot be negative.");

    setAccountId(accountId);
    setAmountPaise(amountPaise);
    setType(type);
    setCategory(std::move(category));
    setDescription(std::move(description));
    setDate(std::move(date));
}

int Transaction::getId() const noexcept
{
    return m_id;
}

int Transaction::getAccountId() const noexcept
{
    return m_accountId;
}

std::int64_t Transaction::getAmountPaise() const noexcept
{
    return m_amountPaise;
}

TransactionType Transaction::getType() const noexcept
{
    return m_type;
}

const std::string& Transaction::getCategory() const noexcept
{
    return m_category;
}

const std::string& Transaction::getDescription() const noexcept
{
    return m_description;
}

const std::string& Transaction::getDate() const noexcept
{
    return m_date;
}

void Transaction::setId(int id)
{
    if (id < 0)
        throw std::invalid_argument("Transaction ID cannot be negative.");

    m_id = id;
}

void Transaction::setAccountId(int accountId)
{
    if (accountId <= 0)
        throw std::invalid_argument("Account ID must be positive.");

    m_accountId = accountId;
}

void Transaction::setAmountPaise(std::int64_t amount)
{
    if (amount <= 0)
        throw std::invalid_argument("Transaction amount must be positive.");

    m_amountPaise = amount;
}

void Transaction::setType(TransactionType type)
{
    validateType(type);
    m_type = type;
}

void Transaction::setCategory(std::string category)
{
    category = trim(category);

    if (category.empty())
        throw std::invalid_argument("Category cannot be empty.");

    m_category = std::move(category);
}

void Transaction::setDescription(std::string description)
{
    description = trim(description);

    if (description.empty())
        throw std::invalid_argument("Description cannot be empty.");

    m_description = std::move(description);
}

void Transaction::setDate(std::string date)
{
    date = trim(date);

    if (!isValidDate(date))
        throw std::invalid_argument("Date must be a valid YYYY-MM-DD date.");

    m_date = std::move(date);
}
