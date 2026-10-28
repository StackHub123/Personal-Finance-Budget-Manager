#ifndef BUDGETSERVICE_H
#define BUDGETSERVICE_H

#include "models/Budget.h"
#include "models/Transaction.h"
#include "repositories/BudgetRepository.h"
#include "repositories/TransactionRepository.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct BudgetStatus
{
    Budget budget;
    std::int64_t spentPaise;
    std::int64_t remainingPaise;
    bool overBudget;
};

class BudgetService
{
public:
    BudgetService(
        BudgetRepository& budgetRepository,
        TransactionRepository& transactionRepository
    );

    // Create a budget and return its generated database ID.
    bool addBudget(
        int accountId,
        std::string category,
        int month,
        int year,
        std::int64_t limitPaise,
        int& createdBudgetId,
        std::string& error
    );

    // Retrieve a budget together with its current spending status.
    std::optional<BudgetStatus> getBudgetStatus(
        int budgetId,
        std::string& error
    ) const;

    // Retrieve every budget together with its current spending status.
    std::vector<BudgetStatus> getAllBudgetStatuses(
        std::string& error
    ) const;

    // Update an existing budget.
    bool updateBudget(
        const Budget& budget,
        std::string& error
    );

    // Delete a budget.
    bool deleteBudget(
        int budgetId,
        std::string& error
    );

private:
    // Calculate spending for one budget using an already-loaded
    // collection of transactions.
    std::optional<BudgetStatus> calculateStatus(
        const Budget& budget,
        const std::vector<Transaction>& transactions,
        std::string& error
    ) const;

    BudgetRepository& m_budgetRepository;
    TransactionRepository& m_transactionRepository;
};

#endif // BUDGETSERVICE_H
