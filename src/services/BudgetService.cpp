#include "services/BudgetService.h"

#include <exception>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

BudgetService::BudgetService(
    BudgetRepository& budgetRepository,
    TransactionRepository& transactionRepository)
    : m_budgetRepository(budgetRepository),
      m_transactionRepository(transactionRepository)
{
}

bool BudgetService::addBudget(
    std::string category,
    int month,
    int year,
    std::int64_t limitPaise,
    int& createdBudgetId,
    std::string& error)
{
    error.clear();
    createdBudgetId = 0;

    try
    {
        Budget budget(
            0,
            std::move(category),
            month,
            year,
            limitPaise
        );

        if (!m_budgetRepository.add(budget, error))
        {
            if (error.empty())
            {
                error = "Could not save the budget.";
            }

            return false;
        }

        createdBudgetId = budget.getId();
        error.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}

std::optional<BudgetStatus> BudgetService::getBudgetStatus(
    int budgetId,
    std::string& error) const
{
    error.clear();

    if (budgetId <= 0)
    {
        error = "Budget ID must be positive.";
        return std::nullopt;
    }

    const auto budget = m_budgetRepository.findById(budgetId, error);

    if (!budget)
    {
        if (error.empty())
        {
            error = "Budget could not be found.";
        }

        return std::nullopt;
    }

    const auto transactions = m_transactionRepository.getAll(error);

    if (!error.empty())
    {
        return std::nullopt;
    }

    return calculateStatus(*budget, transactions, error);
}

std::vector<BudgetStatus> BudgetService::getAllBudgetStatuses(
    std::string& error) const
{
    error.clear();

    const auto budgets = m_budgetRepository.getAll(error);

    if (!error.empty())
    {
        return {};
    }

    const auto transactions = m_transactionRepository.getAll(error);

    if (!error.empty())
    {
        return {};
    }

    std::vector<BudgetStatus> statuses;
    statuses.reserve(budgets.size());

    for (const auto& budget : budgets)
    {
        auto status = calculateStatus(budget, transactions, error);

        if (!status)
        {
            return {};
        }

        statuses.push_back(*status);
    }

    error.clear();
    return statuses;
}

bool BudgetService::updateBudget(
    const Budget& budget,
    std::string& error)
{
    error.clear();

    if (budget.getId() <= 0)
    {
        error = "Budget ID must be positive.";
        return false;
    }

    if (!m_budgetRepository.update(budget, error))
    {
        if (error.empty())
        {
            error = "Could not update the budget.";
        }

        return false;
    }

    error.clear();
    return true;
}

bool BudgetService::deleteBudget(
    int budgetId,
    std::string& error)
{
    error.clear();

    if (budgetId <= 0)
    {
        error = "Budget ID must be positive.";
        return false;
    }

    if (!m_budgetRepository.remove(budgetId, error))
    {
        if (error.empty())
        {
            error = "Could not delete the budget.";
        }

        return false;
    }

    error.clear();
    return true;
}

std::optional<BudgetStatus> BudgetService::calculateStatus(
    const Budget& budget,
    const std::vector<Transaction>& transactions,
    std::string& error) const
{
    error.clear();

    // Build a YYYY-MM prefix to match the budget's month and year
    // against transaction dates stored as YYYY-MM-DD.
    std::ostringstream period;
    period << std::setfill('0')
           << std::setw(4) << budget.getYear()
           << '-'
           << std::setw(2) << budget.getMonth();

    const std::string monthPrefix = period.str();

    std::int64_t spentPaise = 0;

    for (const auto& transaction : transactions)
    {
        // Only expenses in the budget's category count toward spending.
        if (transaction.getType() != TransactionType::Expense ||
            transaction.getCategory() != budget.getCategory())
        {
            continue;
        }

        const std::string& date = transaction.getDate();

        if (date.size() != 10 ||
            date[4] != '-' ||
            date[7] != '-')
        {
            error = "A matching expense has an invalid date.";
            return std::nullopt;
        }

        // Include only transactions in this budget's month and year.
        if (date.compare(0, 7, monthPrefix) != 0)
        {
            continue;
        }

        const std::int64_t amount = transaction.getAmountPaise();

        if (amount <= 0)
        {
            error = "A matching expense has an invalid amount.";
            return std::nullopt;
        }

        // Prevent overflow when adding expense amounts.
        if (spentPaise >
            std::numeric_limits<std::int64_t>::max() - amount)
        {
            error = "Budget spending exceeds the supported integer range.";
            return std::nullopt;
        }

        spentPaise += amount;
    }

    const std::int64_t remainingPaise =
        budget.getLimitPaise() - spentPaise;

    BudgetStatus status{
        budget,
        spentPaise,
        remainingPaise,
        spentPaise > budget.getLimitPaise()
    };

    error.clear();
    return status;
}
