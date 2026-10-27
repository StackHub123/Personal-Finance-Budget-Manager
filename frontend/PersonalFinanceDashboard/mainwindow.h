#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "database/DatabaseManager.h"
#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"
#include "repositories/BudgetRepository.h"
#include "services/FinanceService.h"
#include "services/TransactionService.h"
#include "services/BudgetService.h"
#include "services/BackupRestoreService.h"

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QStringList>

class QButtonGroup;
class QFrame;
class QStackedWidget;
class QLabel;
class QPushButton;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QProgressBar;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QWidget *buildSidebar();
    QWidget *buildDashboardPage();
    QWidget *buildAccountsPage();
    QWidget *buildImportStatementsPage();
    QWidget *buildTransactionsPage();
    QWidget *buildBudgetsPage();
    QWidget *buildReportsPage();
    QWidget *buildUtilitiesPage();
    QWidget *buildPlaceholderPage(const QString &title, const QString &description);
    QWidget *buildMetricCard(const QString &caption,
                             const QString &value,
                             const QString &detail);
    void navigateTo(int pageIndex);
    void refreshAccountsTable();
    void createAccount();
    void viewAccounts();
    void editAccount();
    void deleteAccount();
    void showAccountMessage(const QString &message, bool success);
    QStringList splitCsvLine(const QString &line) const;
    void chooseStatementFile();
    void confirmStatementImport();
    void refreshStatementAccountCombo();
    void clearStatementPreview(const QString &message);
    void refreshTransactionsTable();
    void addTransaction(bool income);
    void editTransaction();
    void deleteTransaction();
    void refreshBudgetsTable();
    void addBudget();
    void editBudget();
    void deleteBudget();
    void refreshDashboard();
    void runReport();
    void backupData();
    void restoreData();
    void applyStyles();

    DatabaseManager m_database;
    AccountRepository m_accountRepository;
    TransactionRepository m_transactionRepository;
    BudgetRepository m_budgetRepository;
    FinanceService m_financeService;
    TransactionService m_transactionService;
    BudgetService m_budgetService;
    BackupRestoreService m_backupRestoreService;
    bool m_databaseReady = false;
    QString m_databaseError;

    QStackedWidget *m_pages = nullptr;
    QButtonGroup *m_navigationGroup = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_accountsTable = nullptr;
    QFrame *m_emptyState = nullptr;
    QPushButton *m_editAccountButton = nullptr;
    QPushButton *m_deleteAccountButton = nullptr;
    QLabel *m_statementPathLabel = nullptr;
    QLabel *m_statementAccountNotice = nullptr;
    QComboBox *m_statementAccountCombo = nullptr;
    QPushButton *m_chooseStatementButton = nullptr;
    QLabel *m_statementDetectedLabel = nullptr;
    QLabel *m_statementErrorLabel = nullptr;
    QLabel *m_statementDuplicateLabel = nullptr;
    QLabel *m_statementErrorDetails = nullptr;
    QTableWidget *m_statementTable = nullptr;
    QPushButton *m_confirmImportButton = nullptr;
    QString m_statementPath;

    struct StatementRecord {
        QString date;
        QString description;
        QString type;
        double amount = 0.0;
        int lineNumber = 0;
        bool valid = false;
        bool duplicate = false;
    };
    struct ImportedTransactionRecord {
        int accountId = -1;
        QString date;
        QString description;
        double amount = 0.0;
    };
    QVector<StatementRecord> m_statementRows;
    QVector<ImportedTransactionRecord> m_importedTransactions;
    int m_statementInvalidRowCount = 0;
    int m_statementAccountId = -1;

    struct AccountRecord {
        int id = 0;
        QString name;
        QString type;
        double balance = 0.0;
    };

    QVector<AccountRecord> m_accounts;
    int m_nextAccountId = 1;

    struct TransactionRecord {
        int id = 0;
        int accountId = -1;
        QString date;
        QString description;
        QString category;
        QString type; // Income or Expense
        double amount = 0.0; // always positive; type determines direction
    };
    QVector<TransactionRecord> m_transactions;
    int m_nextTransactionId = 1;
    QTableWidget *m_transactionsTable = nullptr;
    QLineEdit *m_transactionSearch = nullptr;
    QComboBox *m_transactionTypeFilter = nullptr;
    QComboBox *m_transactionAccountCombo = nullptr;
    QPushButton *m_editTransactionButton = nullptr;
    QPushButton *m_deleteTransactionButton = nullptr;

    struct BudgetRecord {
        int id = 0;
        QString category;
        double limit = 0.0;
        QString month; // YYYY-MM
    };
    QVector<BudgetRecord> m_budgets;
    int m_nextBudgetId = 1;
    QTableWidget *m_budgetsTable = nullptr;
    QPushButton *m_editBudgetButton = nullptr;
    QPushButton *m_deleteBudgetButton = nullptr;

    QComboBox *m_reportTypeCombo = nullptr;
    QComboBox *m_reportPeriodCombo = nullptr;
    QTableWidget *m_reportTable = nullptr;
    QLabel *m_reportSummary = nullptr;
};

#endif // MAINWINDOW_H
