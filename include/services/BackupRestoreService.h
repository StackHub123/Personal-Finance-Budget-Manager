#ifndef BACKUPRESTORESERVICE_H
#define BACKUPRESTORESERVICE_H

#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "models/Transaction.h"
#include "models/Budget.h"
#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"
#include "repositories/BudgetRepository.h"

#include <string>
#include <vector>

class BackupRestoreService
{
public:
    BackupRestoreService(
        DatabaseManager& database,
        AccountRepository& accounts,
        TransactionRepository& transactions,
        BudgetRepository& budgets);

    // Atomically replace the database contents with restored records.
    bool replaceAll(
        const std::vector<Account>& accounts,
        const std::vector<Transaction>& transactions,
        const std::vector<Budget>& budgets,
        std::string& error);

private:
    DatabaseManager& m_database;
    AccountRepository& m_accounts;
    TransactionRepository& m_transactions;
    BudgetRepository& m_budgets;
};

#endif
