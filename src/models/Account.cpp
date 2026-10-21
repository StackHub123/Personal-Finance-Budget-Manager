#include "models/Account.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace
{
    std::string trim(const std::string& value)
    {
        const auto first = std::find_if_not(
            value.begin(),
            value.end(),
            [](unsigned char ch) {
                return std::isspace(ch) != 0;
            });

        const auto last = std::find_if_not(
            value.rbegin(),
            value.rend(),
            [](unsigned char ch) {
                return std::isspace(ch) != 0;
            }).base();

        if (first >= last)
        {
            return {};
        }

        return std::string(first, last);
    }
}

Account::Account(int id,
                 std::string name,
                 std::string type,
                 std::int64_t initialBalancePaise)
    : m_id(id),
      m_initialBalancePaise(initialBalancePaise)
{
    if (id < 0)
    {
        throw std::invalid_argument(
            "Account ID cannot be negative.");
    }

    setName(std::move(name));
    setType(std::move(type));
}

int Account::getId() const noexcept
{
    return m_id;
}

const std::string& Account::getName() const noexcept
{
    return m_name;
}

const std::string& Account::getType() const noexcept
{
    return m_type;
}

std::int64_t Account::getInitialBalancePaise() const noexcept
{
    return m_initialBalancePaise;
}

void Account::setId(int id)
{
    if (id < 0)
    {
        throw std::invalid_argument(
            "Account ID cannot be negative.");
    }

    m_id = id;
}

void Account::setName(std::string name)
{
    name = trim(name);

    if (name.empty())
    {
        throw std::invalid_argument(
            "Account name cannot be empty.");
    }

    m_name = std::move(name);
}

void Account::setType(std::string type)
{
    type = trim(type);

    if (type.empty())
    {
        throw std::invalid_argument(
            "Account type cannot be empty.");
    }

    m_type = std::move(type);
}

void Account::setInitialBalancePaise(
    std::int64_t amount) noexcept
{
    m_initialBalancePaise = amount;
}
