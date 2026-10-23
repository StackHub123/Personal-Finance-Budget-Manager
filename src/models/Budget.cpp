#include "models/Budget.h"

#include <stdexcept>
#include <string>

namespace
{
    std::string trim(const std::string& value)
    {
        const std::string whitespace = " \t\n\r\f\v";

        const auto first = value.find_first_not_of(whitespace);

        if (first == std::string::npos)
        {
            return "";
        }

        const auto last = value.find_last_not_of(whitespace);

        return value.substr(first, last - first + 1);
    }
}

Budget::Budget(int id,
               std::string category,
               int month,
               int year,
               std::int64_t limitPaise)
    : m_id(id),
      m_category(trim(category)),
      m_month(month),
      m_year(year),
      m_limitPaise(limitPaise)
{
    if (m_id < 0)
    {
        throw std::invalid_argument("Budget ID cannot be negative.");
    }

    if (m_category.empty())
    {
        throw std::invalid_argument("Budget category cannot be empty.");
    }

    if (m_month < 1 || m_month > 12)
    {
        throw std::invalid_argument("Budget month must be between 1 and 12.");
    }

    if (m_year < 1)
    {
        throw std::invalid_argument("Budget year must be positive.");
    }

    if (m_limitPaise <= 0)
    {
        throw std::invalid_argument("Budget limit must be greater than zero.");
    }
}

int Budget::getId() const noexcept
{
    return m_id;
}

const std::string& Budget::getCategory() const noexcept
{
    return m_category;
}

int Budget::getMonth() const noexcept
{
    return m_month;
}

int Budget::getYear() const noexcept
{
    return m_year;
}

std::int64_t Budget::getLimitPaise() const noexcept
{
    return m_limitPaise;
}

void Budget::setId(int id)
{
    if (id < 0)
    {
        throw std::invalid_argument("Budget ID cannot be negative.");
    }

    m_id = id;
}

void Budget::setCategory(std::string category)
{
    category = trim(category);

    if (category.empty())
    {
        throw std::invalid_argument("Budget category cannot be empty.");
    }

    m_category = category;
}

void Budget::setMonth(int month)
{
    if (month < 1 || month > 12)
    {
        throw std::invalid_argument("Budget month must be between 1 and 12.");
    }

    m_month = month;
}

void Budget::setYear(int year)
{
    if (year < 1)
    {
        throw std::invalid_argument("Budget year must be positive.");
    }

    m_year = year;
}

void Budget::setLimitPaise(std::int64_t limitPaise)
{
    if (limitPaise <= 0)
    {
        throw std::invalid_argument("Budget limit must be greater than zero.");
    }

    m_limitPaise = limitPaise;
}
