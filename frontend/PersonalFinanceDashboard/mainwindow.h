#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>

class QButtonGroup;
class QFrame;
class QStackedWidget;
class QLabel;
class QPushButton;
class QTableWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QWidget *buildSidebar();
    QWidget *buildDashboardPage();
    QWidget *buildAccountsPage();
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
    void applyStyles();

    QStackedWidget *m_pages = nullptr;
    QButtonGroup *m_navigationGroup = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_accountsTable = nullptr;
    QFrame *m_emptyState = nullptr;
    QPushButton *m_editAccountButton = nullptr;
    QPushButton *m_deleteAccountButton = nullptr;

    struct AccountRecord {
        int id = 0;
        QString name;
        QString type;
        double balance = 0.0;
    };

    QVector<AccountRecord> m_accounts;
    int m_nextAccountId = 1;
};

#endif // MAINWINDOW_H
