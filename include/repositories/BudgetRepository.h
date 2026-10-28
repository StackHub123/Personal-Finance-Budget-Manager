#ifndef BUDGETREPOSITORY_H
#define BUDGETREPOSITORY_H

#include "database/DatabaseManager.h"
#include "models/Budget.h"

#include <optional>
#include <string>
#include <vector>

class BudgetRepository
{
public:
    explicit BudgetRepository(DatabaseManager& database);

    // Save a budget and assign its generated database ID.
    bool add(Budget& budget, std::string& error);

    // Retrieve one budget by ID.
    std::optional<Budget> findById(int id, std::string& error) const;

    // Retrieve all budgets (including legacy budgets not yet assigned).
    std::vector<Budget> getAll(std::string& error) const;

    // Retrieve budgets owned by one account.
    std::vector<Budget> getByAccountId(int accountId, std::string& error) const;

    // Update an existing budget.
    bool update(const Budget& budget, std::string& error);

    // Delete a budget by ID.
    bool remove(int id, std::string& error);

    // Restore a budget with its original database ID.
    bool addRestored(const Budget& budget, std::string& error);

private:
    DatabaseManager& m_database;
};

#endif // BUDGETREPOSITORY_H
