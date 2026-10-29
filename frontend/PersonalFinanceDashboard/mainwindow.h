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
#include "models/AuthenticatedUser.h"

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
class QVBoxLayout;
class DonutChartWidget;
class HorizontalBarChartWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const AuthenticatedUser &user,
                        const QString &databasePath,
                        QWidget *parent = nullptr);
    bool isDatabaseReady() const noexcept { return m_databaseReady; }
    QString databaseError() const { return m_databaseError; }
    bool importLegacyData(const QString &legacyDatabasePath, QString &error);

signals:
    void logoutRequested();

private:
    QWidget *buildSidebar();
    QWidget *buildDashboardPage();
    QWidget *buildAccountsPage();
    QWidget *buildImportStatementsPage();
    QWidget *buildTransactionsPage();
    QWidget *buildBudgetsPage();
    QWidget *buildReportsPage();
    QWidget *buildUtilitiesPage();
    QWidget *buildSettingsPage();
    QWidget *buildPlaceholderPage(const QString &title, const QString &description);
    QWidget *buildMetricCard(const QString &caption,
                             const QString &value,
                             const QString &detail);
    void navigateTo(int pageIndex);
    void refreshAccountsTable();
    void refreshActiveAccountCombo();
    void activeAccountChanged(int index);
    void refreshAccountScopedViews();
    void createAccount();
    void viewAccounts();
    void deleteAccount();
    void showAccountMessage(const QString &message, bool success);
    QStringList splitCsvLine(const QString &line, QChar delimiter = ',') const;
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
    QString formatMoney(double amount) const;
    QString formatDate(const QString &isoDate) const;
    void applyDisplayPreferences();
    QString userSettingKey(const QString &key) const;
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
    AuthenticatedUser m_authenticatedUser;
    bool m_databaseReady = false;
    QString m_databaseError;

    QStackedWidget *m_pages = nullptr;
    QButtonGroup *m_navigationGroup = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_statusLabel = nullptr;
    QComboBox *m_activeAccountCombo = nullptr;
    int m_activeAccountId = 0;
    QString m_themePreference = "light";
    QString m_dateDisplayFormat = "yyyy-MM-dd";
    QString m_numberFormat = "indian";
    QString m_defaultReportPeriod = "This month";
    QString m_importDuplicatePolicy = "flag";
    QComboBox *m_settingsThemeCombo = nullptr;
    QComboBox *m_settingsDateFormatCombo = nullptr;
    QComboBox *m_settingsNumberFormatCombo = nullptr;
    QComboBox *m_settingsReportPeriodCombo = nullptr;
    QComboBox *m_settingsDuplicatePolicyCombo = nullptr;
    QTableWidget *m_accountsTable = nullptr;
    QFrame *m_emptyState = nullptr;
    QPushButton *m_deleteAccountButton = nullptr;
    QLabel *m_accountsSummaryLabel = nullptr;
    QLabel *m_accountDetailsLabel = nullptr;
    QLabel *m_signedInEmailLabel = nullptr;
    QPushButton *m_logoutButton = nullptr;
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
        QString duplicateReason;
    };
    QVector<StatementRecord> m_statementRows;
    int m_statementInvalidRowCount = 0;
    int m_statementAccountId = -1;

    struct AccountRecord {
        int id = 0;
        QString name;
        QString type;
        double balance = 0.0; // opening balance, used with transaction aggregates
        double currentBalance = 0.0;
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
        int accountId = 0; // 0 means a legacy budget not yet assigned
        QString accountName;
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
    DonutChartWidget *m_reportDonutChart = nullptr;
    HorizontalBarChartWidget *m_reportBarChart = nullptr;
    QLabel *m_reportDonutTitle = nullptr;
    QLabel *m_reportBarTitle = nullptr;
    QFrame *m_reportBudgetPanel = nullptr;
    QVBoxLayout *m_reportBudgetRowsLayout = nullptr;
    bool m_reportWasGenerated = false;
};

#endif // MAINWINDOW_H
