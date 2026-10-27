#include "services/BackupRestoreService.h"

#include <set>
#include <string>
#include <utility>

BackupRestoreService::BackupRestoreService(
    DatabaseManager& database,
    AccountRepository& accounts,
    TransactionRepository& transactions,
    BudgetRepository& budgets)
    : m_database(database),
      m_accounts(accounts),
      m_transactions(transactions),
      m_budgets(budgets)
{
}

bool BackupRestoreService::replaceAll(
    const std::vector<Account>& accounts,
    const std::vector<Transaction>& transactions,
    const std::vector<Budget>& budgets,
    std::string& error)
{
    error.clear();

    if (!m_database.isOpen()) {
        error = "Cannot restore data: database is not open.";
        return false;
    }

    // Validate IDs and relationships before touching existing data.
    std::set<int> accountIds;
    std::set<int> transactionIds;
    std::set<int> budgetIds;

    for (const auto& account : accounts) {
        if (account.getId() <= 0 ||
            !accountIds.insert(account.getId()).second) {
            error = "Backup contains an invalid or repeated account ID.";
            return false;
        }
    }

    for (const auto& transaction : transactions) {
        if (transaction.getId() <= 0 ||
            !transactionIds.insert(transaction.getId()).second) {
            error = "Backup contains an invalid or repeated transaction ID.";
            return false;
        }

        if (accountIds.find(transaction.getAccountId()) == accountIds.end()) {
            error = "A transaction references an account missing from the backup.";
            return false;
        }

        if (transaction.getAmountPaise() <= 0) {
            error = "Backup contains a non-positive transaction amount.";
            return false;
        }
    }

    for (const auto& budget : budgets) {
        if (budget.getId() <= 0 ||
            !budgetIds.insert(budget.getId()).second) {
            error = "Backup contains an invalid or repeated budget ID.";
            return false;
        }

        if (budget.getCategory().empty() ||
            budget.getMonth() < 1 || budget.getMonth() > 12 ||
            budget.getYear() <= 0 ||
            budget.getLimitPaise() <= 0) {
            error = "Backup contains an invalid budget.";
            return false;
        }
    }

    if (!m_database.execute("BEGIN IMMEDIATE;", error)) {
        return false;
    }

    auto rollback = [&]() {
        std::string ignored;
        m_database.execute("ROLLBACK;", ignored);
    };

    // Fetch existing rows while the transaction is active.
    error.clear();
    const auto oldTransactions = m_transactions.getAll(error);
    if (!error.empty()) {
        rollback();
        return false;
    }

    error.clear();
    const auto oldBudgets = m_budgets.getAll(error);
    if (!error.empty()) {
        rollback();
        return false;
    }

    error.clear();
    const auto oldAccounts = m_accounts.getAll(error);
    if (!error.empty()) {
        rollback();
        return false;
    }

    // Delete in foreign-key-safe order.
    for (const auto& item : oldTransactions) {
        error.clear();
        if (!m_transactions.remove(item.getId(), error)) {
            if (error.empty())
                error = "Could not remove an existing transaction.";
            rollback();
            return false;
        }
    }

    for (const auto& item : oldBudgets) {
        error.clear();
        if (!m_budgets.remove(item.getId(), error)) {
            if (error.empty())
                error = "Could not remove an existing budget.";
            rollback();
            return false;
        }
    }

    for (const auto& item : oldAccounts) {
        error.clear();
        if (!m_accounts.remove(item.getId(), error)) {
            if (error.empty())
                error = "Could not remove an existing account.";
            rollback();
            return false;
        }
    }

    // Restore parent records first, preserving the original IDs.
    for (const auto& item : accounts) {
        error.clear();
        if (!m_accounts.addRestored(item, error)) {
            rollback();
            return false;
        }
    }

    for (const auto& item : transactions) {
        error.clear();
        if (!m_transactions.addRestored(item, error)) {
            rollback();
            return false;
        }
    }

    for (const auto& item : budgets) {
        error.clear();
        if (!m_budgets.addRestored(item, error)) {
            rollback();
            return false;
        }
    }

    error.clear();
    if (!m_database.execute("COMMIT;", error)) {
        rollback();
        return false;
    }

    error.clear();
    return true;
}
