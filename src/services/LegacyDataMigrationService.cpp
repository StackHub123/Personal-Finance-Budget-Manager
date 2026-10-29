#include "services/LegacyDataMigrationService.h"

#include "services/BackupRestoreService.h"

#include <sqlite3.h>

#include <QTemporaryDir>

#include <memory>
#include <string>
#include <vector>

namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

bool tableExists(sqlite3 *database, const char *tableName, bool &exists)
{
    sqlite3_stmt *raw = nullptr;
    if (sqlite3_prepare_v2(database,
                           "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?;",
                           -1, &raw, nullptr) != SQLITE_OK)
        return false;
    Statement statement(raw, sqlite3_finalize);
    sqlite3_bind_text(statement.get(), 1, tableName, -1, SQLITE_STATIC);
    const int result = sqlite3_step(statement.get());
    if (result != SQLITE_ROW && result != SQLITE_DONE)
        return false;
    exists = result == SQLITE_ROW;
    return true;
}

bool hasRows(sqlite3 *database, const char *tableName, bool &hasRowsResult)
{
    const std::string sql = std::string("SELECT EXISTS(SELECT 1 FROM ") + tableName + ");";
    sqlite3_stmt *raw = nullptr;
    if (sqlite3_prepare_v2(database, sql.c_str(), -1, &raw, nullptr) != SQLITE_OK)
        return false;
    Statement statement(raw, sqlite3_finalize);
    if (sqlite3_step(statement.get()) != SQLITE_ROW)
        return false;
    hasRowsResult = sqlite3_column_int(statement.get(), 0) != 0;
    return true;
}

bool copyDatabaseSnapshot(const QString &sourcePath,
                          const QString &destinationPath)
{
    sqlite3 *source = nullptr;
    sqlite3 *destination = nullptr;
    const QByteArray sourceName = sourcePath.toUtf8();
    const QByteArray destinationName = destinationPath.toUtf8();
    if (sqlite3_open_v2(sourceName.constData(), &source,
                        SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        if (source)
            sqlite3_close_v2(source);
        return false;
    }
    if (sqlite3_open_v2(destinationName.constData(), &destination,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                            SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        sqlite3_close_v2(source);
        if (destination)
            sqlite3_close_v2(destination);
        return false;
    }

    sqlite3_backup *backup = sqlite3_backup_init(destination, "main", source, "main");
    bool copied = false;
    if (backup) {
        const int stepResult = sqlite3_backup_step(backup, -1);
        const int finishResult = sqlite3_backup_finish(backup);
        copied = stepResult == SQLITE_DONE && finishResult == SQLITE_OK;
    }
    const int destinationCloseResult = sqlite3_close_v2(destination);
    const int sourceCloseResult = sqlite3_close_v2(source);
    const bool closed = destinationCloseResult == SQLITE_OK &&
                        sourceCloseResult == SQLITE_OK;
    return copied && closed;
}
}

bool LegacyDataMigrationService::containsFinancialRecords(
    const QString &databasePath, bool &containsRecords, QString &error)
{
    containsRecords = false;
    error.clear();
    sqlite3 *database = nullptr;
    const QByteArray path = databasePath.toUtf8();
    if (sqlite3_open_v2(path.constData(), &database,
                        SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        if (database)
            sqlite3_close_v2(database);
        error = QStringLiteral("Existing financial data could not be inspected safely.");
        return false;
    }

    for (const char *tableName : {"accounts", "transactions", "budgets"}) {
        bool exists = false;
        if (!tableExists(database, tableName, exists)) {
            sqlite3_close_v2(database);
            error = QStringLiteral("Existing financial data could not be inspected safely.");
            return false;
        }
        if (exists && !hasRows(database, tableName, containsRecords)) {
            sqlite3_close_v2(database);
            error = QStringLiteral("Existing financial data could not be inspected safely.");
            return false;
        }
        if (containsRecords)
            break;
    }
    sqlite3_close_v2(database);
    return true;
}

bool LegacyDataMigrationService::importInto(
    const QString &legacyDatabasePath,
    DatabaseManager &targetDatabase,
    AccountRepository &targetAccounts,
    TransactionRepository &targetTransactions,
    BudgetRepository &targetBudgets,
    QString &error)
{
    QTemporaryDir snapshotDirectory;
    if (!snapshotDirectory.isValid()) {
        error = QStringLiteral("A safe temporary copy could not be created. "
                              "The original database has been left unchanged.");
        return false;
    }
    const QString snapshotPath = snapshotDirectory.filePath("legacy_snapshot.sqlite");
    if (!copyDatabaseSnapshot(legacyDatabasePath, snapshotPath)) {
        error = QStringLiteral("A consistent copy of the existing data could not be made. "
                              "The original database has been left unchanged.");
        return false;
    }

    DatabaseManager sourceDatabase;
    std::string dbError;
    if (!sourceDatabase.open(snapshotPath.toStdString(), dbError) ||
        !sourceDatabase.initializeSchema(dbError)) {
        error = QStringLiteral("The existing data could not be prepared for a safe import. "
                              "The original database has been left in place.");
        return false;
    }

    AccountRepository sourceAccounts(sourceDatabase);
    TransactionRepository sourceTransactions(sourceDatabase);
    BudgetRepository sourceBudgets(sourceDatabase);
    auto accounts = sourceAccounts.getAll(dbError);
    if (!dbError.empty()) {
        error = QStringLiteral("The existing financial data could not be read.");
        return false;
    }
    auto transactions = sourceTransactions.getAll(dbError);
    if (!dbError.empty()) {
        error = QStringLiteral("The existing financial data could not be read.");
        return false;
    }
    auto budgets = sourceBudgets.getAll(dbError);
    if (!dbError.empty()) {
        error = QStringLiteral("The existing financial data could not be read.");
        return false;
    }

    BackupRestoreService restore(targetDatabase, targetAccounts,
                                 targetTransactions, targetBudgets);
    if (!restore.replaceAll(accounts, transactions, budgets, dbError)) {
        error = QStringLiteral("The existing data could not be imported completely. "
                              "Your original database remains unchanged.");
        return false;
    }
    error.clear();
    return true;
}
