#ifndef LEGACYDATAMIGRATIONSERVICE_H
#define LEGACYDATAMIGRATIONSERVICE_H

#include "database/DatabaseManager.h"
#include "repositories/AccountRepository.h"
#include "repositories/BudgetRepository.h"
#include "repositories/TransactionRepository.h"

#include <QString>

class LegacyDataMigrationService final
{
public:
    static bool containsFinancialRecords(const QString &databasePath,
                                         bool &containsRecords,
                                         QString &error);
    static bool importInto(const QString &legacyDatabasePath,
                           DatabaseManager &targetDatabase,
                           AccountRepository &targetAccounts,
                           TransactionRepository &targetTransactions,
                           BudgetRepository &targetBudgets,
                           QString &error);
};

#endif // LEGACYDATAMIGRATIONSERVICE_H
