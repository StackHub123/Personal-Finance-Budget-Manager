#include "mainwindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QSignalBlocker>
#include <QTextStream>
#include <QDate>
#include <QSet>
#include <QRegularExpression>
#include <algorithm>
#include <QtMath>
#include <QGroupBox>
#include <QDateEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QDate>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QColor>

namespace {
const QStringList navigationItems = {
    "Dashboard", "Transactions", "Accounts", "Budgets",
    "Reports", "Import statements", "Backup & restore", "Settings"
};

QLabel *makeLabel(const QString &text, const QString &objectName = {})
{
    auto *label = new QLabel(text);
    if (!objectName.isEmpty())
        label->setObjectName(objectName);
    return label;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Personal Finance & Budgeting Dashboard");
    resize(1240, 780);
    setMinimumSize(960, 620);

    auto *central = new QWidget(this);
    central->setObjectName("centralSurface");
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    rootLayout->addWidget(buildSidebar());

    auto *mainArea = new QWidget;
    auto *mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(30, 24, 30, 24);
    mainLayout->setSpacing(18);

    auto *header = new QHBoxLayout;
    auto *headingColumn = new QVBoxLayout;
    m_pageTitle = makeLabel("Dashboard", "pageTitle");
    auto *subtitle = makeLabel("A clear view of your money.");
    subtitle->setObjectName("pageSubtitle");
    headingColumn->addWidget(m_pageTitle);
    headingColumn->addWidget(subtitle);
    header->addLayout(headingColumn);
    header->addStretch();

    auto *periodLabel = makeLabel("OVERVIEW  ·  DEMO DATA");
    periodLabel->setObjectName("periodTag");
    header->addWidget(periodLabel, 0, Qt::AlignTop);
    mainLayout->addLayout(header);

    m_pages = new QStackedWidget;
    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildTransactionsPage());
    m_pages->addWidget(buildAccountsPage());
    m_pages->addWidget(buildBudgetsPage());
    m_pages->addWidget(buildReportsPage());
    m_pages->addWidget(buildImportStatementsPage());
    m_pages->addWidget(buildUtilitiesPage());
    m_pages->addWidget(buildUtilitiesPage());

    mainLayout->addWidget(m_pages, 1);

    m_statusLabel = makeLabel(
        "Frontend prototype • Demo figures only • No database connection yet",
        "statusLabel");
    mainLayout->addWidget(m_statusLabel);

    rootLayout->addWidget(mainArea, 1);
    setCentralWidget(central);

    applyStyles();
    navigateTo(0);
}

QWidget *MainWindow::buildSidebar()
{
    auto *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(238);

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(18, 24, 18, 18);
    layout->setSpacing(8);

    auto *brand = makeLabel("PFBM", "brandMark");
    auto *brandSubtitle = makeLabel("PERSONAL FINANCE", "brandSubtitle");
    layout->addWidget(brand);
    layout->addWidget(brandSubtitle);
    layout->addSpacing(24);

    auto *sectionLabel = makeLabel("WORKSPACE", "sidebarSectionLabel");
    layout->addWidget(sectionLabel);

    m_navigationGroup = new QButtonGroup(this);
    m_navigationGroup->setExclusive(true);

    for (int i = 0; i < navigationItems.size(); ++i) {
        auto *button = new QPushButton(navigationItems.at(i));
        button->setObjectName("navigationButton");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        m_navigationGroup->addButton(button, i);
        layout->addWidget(button);
    }

    connect(m_navigationGroup, &QButtonGroup::idClicked,
            this, &MainWindow::navigateTo);

    layout->addStretch();

    auto *note = new QFrame;
    note->setObjectName("sidebarNote");
    auto *noteLayout = new QVBoxLayout(note);
    auto *noteTitle = makeLabel("Your money, organised", "sidebarNoteTitle");
    auto *noteBody = makeLabel(
        "Track spending, plan budgets, and understand your finances in one place.",
        "sidebarNoteBody");
    noteBody->setWordWrap(true);
    noteLayout->addWidget(noteTitle);
    noteLayout->addWidget(noteBody);
    layout->addWidget(note);

    auto *version = makeLabel("PFBM  ·  FRONTEND 0.1", "versionLabel");
    layout->addSpacing(10);
    layout->addWidget(version);

    return sidebar;
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(16);

    auto *welcome = new QFrame; welcome->setObjectName("welcomePanel");
    auto *wl = new QVBoxLayout(welcome); wl->setContentsMargins(22, 20, 22, 20);
    wl->addWidget(makeLabel("Your finances at a glance", "welcomeTitle"));
    auto *welcomeText = makeLabel("Figures are calculated from the accounts, transactions and budgets currently in this session. Export a backup from Utilities to keep a copy.", "welcomeText");
    welcomeText->setWordWrap(true); wl->addWidget(welcomeText); layout->addWidget(welcome);

    double accountBalance=0, income=0, expenses=0;
    for (const auto &a : m_accounts) accountBalance += a.balance;
    for (const auto &t : m_transactions) { if(t.type=="Income") income+=t.amount; else expenses+=t.amount; }
    const double currentBalance = accountBalance + income - expenses;
    auto *metrics = new QHBoxLayout; metrics->setSpacing(12);
    metrics->addWidget(buildMetricCard("CURRENT BALANCE", QString("₹ %1").arg(currentBalance,0,'f',2), "Opening balances + transactions"));
    metrics->addWidget(buildMetricCard("TOTAL INCOME", QString("₹ %1").arg(income,0,'f',2), "Recorded income"));
    metrics->addWidget(buildMetricCard("TOTAL EXPENSES", QString("₹ %1").arg(expenses,0,'f',2), "Recorded expenses"));
    metrics->addWidget(buildMetricCard("NET SAVINGS", QString("₹ %1").arg(income-expenses,0,'f',2), "Income minus expenses"));
    layout->addLayout(metrics);

    auto *lower = new QHBoxLayout; lower->setSpacing(14);
    auto *activityPanel = new QFrame; activityPanel->setObjectName("contentPanel");
    auto *al = new QVBoxLayout(activityPanel); al->setContentsMargins(18,18,18,18);
    al->addWidget(makeLabel("Recent activity", "panelTitle"));
    auto *activity = new QTableWidget(0,4); activity->setObjectName("accountsTable");
    activity->setHorizontalHeaderLabels({"Date","Description","Category","Amount"});
    activity->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    activity->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QVector<TransactionRecord> recent=m_transactions;
    std::sort(recent.begin(),recent.end(),[](const TransactionRecord &a,const TransactionRecord &b){return a.date>b.date;});
    for(int i=0;i<qMin(5,recent.size());++i){const auto &t=recent[i];int r=activity->rowCount();activity->insertRow(r);activity->setItem(r,0,new QTableWidgetItem(t.date));activity->setItem(r,1,new QTableWidgetItem(t.description));activity->setItem(r,2,new QTableWidgetItem(t.category));activity->setItem(r,3,new QTableWidgetItem(QString("%1 ₹ %2").arg(t.type=="Income"?"+":"−").arg(t.amount,0,'f',2)));}
    if(recent.isEmpty()) al->addWidget(makeLabel("No transactions yet. Add income or an expense to see your activity.","panelHint"));
    al->addWidget(activity,1);

    auto *budgetPanel = new QFrame; budgetPanel->setObjectName("contentPanel");
    auto *bl = new QVBoxLayout(budgetPanel); bl->setContentsMargins(18,18,18,18);
    bl->addWidget(makeLabel("Budget usage", "panelTitle"));
    bool overBudget=false;
    for(const auto &b:m_budgets){
        double spent=0;for(const auto &t:m_transactions)if(t.type=="Expense"&&t.category.compare(b.category,Qt::CaseInsensitive)==0&&t.date.left(7)==b.month)spent+=t.amount;
        auto *label=makeLabel(QString("%1 · ₹ %2 / ₹ %3").arg(b.category).arg(spent,0,'f',2).arg(b.limit,0,'f',2),"budgetRowTitle");label->setWordWrap(true);bl->addWidget(label);
        auto *bar=new QProgressBar;bar->setRange(0,100);bar->setValue(b.limit>0?qBound(0,static_cast<int>(spent/b.limit*100),100):0);bar->setFormat("%p% used");bl->addWidget(bar);
        if(spent>b.limit){overBudget=true;auto *warning=makeLabel("Over budget: "+b.category,"panelHint");warning->setStyleSheet("color:#b42318;font-weight:700;");bl->addWidget(warning);}
    }
    if(m_budgets.isEmpty())bl->addWidget(makeLabel("No budgets yet. Create a budget to track category spending.","panelHint"));
    if(!overBudget&&!m_budgets.isEmpty())bl->addWidget(makeLabel("No budgets are over their limits.","panelHint"));
    lower->addWidget(activityPanel,3);lower->addWidget(budgetPanel,2);layout->addLayout(lower,1);
    return page;
}

QWidget *MainWindow::buildAccountsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    auto *intro = new QFrame;
    intro->setObjectName("contentPanel");
    auto *introLayout = new QVBoxLayout(intro);
    introLayout->setContentsMargins(20, 18, 20, 18);
    introLayout->setSpacing(6);
    introLayout->addWidget(makeLabel("Account management", "panelTitle"));
    auto *hint = makeLabel(
        "Add the places where you keep or manage money, such as a bank account or cash wallet.",
        "panelHint");
    hint->setWordWrap(true);
    introLayout->addWidget(hint);
    layout->addWidget(intro);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(10);
    auto *createButton = new QPushButton("＋  Create account");
    createButton->setObjectName("primaryActionButton");
    createButton->setCursor(Qt::PointingHandCursor);
    auto *viewButton = new QPushButton("Refresh accounts");
    viewButton->setObjectName("secondaryActionButton");
    viewButton->setCursor(Qt::PointingHandCursor);
    m_editAccountButton = new QPushButton("Edit selected");
    m_editAccountButton->setObjectName("secondaryActionButton");
    m_editAccountButton->setCursor(Qt::PointingHandCursor);
    m_editAccountButton->setEnabled(false);
    m_deleteAccountButton = new QPushButton("Delete selected");
    m_deleteAccountButton->setObjectName("dangerActionButton");
    m_deleteAccountButton->setCursor(Qt::PointingHandCursor);
    m_deleteAccountButton->setEnabled(false);
    actions->addWidget(createButton);
    actions->addWidget(viewButton);
    actions->addWidget(m_editAccountButton);
    actions->addWidget(m_deleteAccountButton);
    actions->addStretch();
    layout->addLayout(actions);

    m_emptyState = new QFrame;
    m_emptyState->setObjectName("emptyStatePanel");
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(24, 28, 24, 28);
    emptyLayout->setSpacing(8);
    auto *emptyTitle = makeLabel("Let's add your first account", "emptyStateTitle");
    auto *emptyText = makeLabel(
        "An account is a place where you keep money, such as your bank account or wallet. "
        "Add one to start tracking your finances.", "panelHint");
    emptyText->setWordWrap(true);
    emptyText->setAlignment(Qt::AlignCenter);
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyText);
    auto *emptyCreateButton = new QPushButton("Create my first account");
    emptyCreateButton->setObjectName("primaryActionButton");
    emptyCreateButton->setCursor(Qt::PointingHandCursor);
    emptyLayout->addWidget(emptyCreateButton, 0, Qt::AlignHCenter);
    layout->addWidget(m_emptyState);

    m_accountsTable = new QTableWidget(0, 4);
    m_accountsTable->setObjectName("accountsTable");
    m_accountsTable->setHorizontalHeaderLabels(
        {"Account ID", "Account name", "Account type", "Starting balance"});
    m_accountsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_accountsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_accountsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_accountsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_accountsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_accountsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_accountsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_accountsTable->setAlternatingRowColors(true);
    m_accountsTable->verticalHeader()->setVisible(false);
    m_accountsTable->setMinimumHeight(260);
    layout->addWidget(m_accountsTable, 1);

    auto *storageNote = makeLabel(
        "Note: account changes are currently temporary and reset when the application closes. "
        "Database saving will be added in a later step.", "panelHint");
    storageNote->setWordWrap(true);
    layout->addWidget(storageNote);

    connect(createButton, &QPushButton::clicked, this, &MainWindow::createAccount);
    connect(emptyCreateButton, &QPushButton::clicked, this, &MainWindow::createAccount);
    connect(viewButton, &QPushButton::clicked, this, &MainWindow::viewAccounts);
    connect(m_editAccountButton, &QPushButton::clicked, this, &MainWindow::editAccount);
    connect(m_deleteAccountButton, &QPushButton::clicked, this, &MainWindow::deleteAccount);
    connect(m_accountsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool selected = m_accountsTable && m_accountsTable->currentRow() >= 0;
        if (m_editAccountButton) m_editAccountButton->setEnabled(selected);
        if (m_deleteAccountButton) m_deleteAccountButton->setEnabled(selected);
    });

    refreshAccountsTable();
    return page;
}

void MainWindow::refreshAccountsTable()
{
    if (!m_accountsTable)
        return;

    m_accountsTable->setRowCount(0);
    for (const AccountRecord &account : m_accounts) {
        const int row = m_accountsTable->rowCount();
        m_accountsTable->insertRow(row);
        auto *idItem = new QTableWidgetItem(QString::number(account.id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_accountsTable->setItem(row, 0, idItem);
        m_accountsTable->setItem(row, 1, new QTableWidgetItem(account.name));
        m_accountsTable->setItem(row, 2, new QTableWidgetItem(account.type));
        auto *balanceItem = new QTableWidgetItem(QString("₹ %1").arg(account.balance, 0, 'f', 2));
        balanceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_accountsTable->setItem(row, 3, balanceItem);
    }

    const bool hasAccounts = !m_accounts.isEmpty();
    m_accountsTable->setVisible(hasAccounts);
    if (m_emptyState) m_emptyState->setVisible(!hasAccounts);
    if (m_editAccountButton) m_editAccountButton->setEnabled(false);
    if (m_deleteAccountButton) m_deleteAccountButton->setEnabled(false);
    refreshStatementAccountCombo();
}

void MainWindow::showAccountMessage(const QString &message, bool success)
{
    if (m_statusLabel)
        m_statusLabel->setText(message);

    if (success)
        QMessageBox::information(this, "Account management", message);
    else
        QMessageBox::warning(this, "Account management", message);
}

void MainWindow::createAccount()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Create an account");
    dialog.setMinimumWidth(460);

    auto *layout = new QVBoxLayout(&dialog);
    auto *intro = makeLabel("Add a place where you keep or manage money.", "panelHint");
    intro->setWordWrap(true);
    layout->addWidget(intro);
    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto *nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("e.g. SBI Savings or My Wallet");
    auto *nameHint = makeLabel("Choose a name you'll recognise.", "fieldHint");
    nameHint->setWordWrap(true);
    auto *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"Savings account", "Cash / Wallet", "Current account", "Credit card", "Other"});
    auto *typeHint = makeLabel(
        "Choose the option that best describes this account. For everyday cash, choose Cash / Wallet.",
        "fieldHint");
    typeHint->setWordWrap(true);
    auto *balanceSpin = new QDoubleSpinBox(&dialog);
    balanceSpin->setRange(0.0, 1000000000000.0);
    balanceSpin->setDecimals(2);
    balanceSpin->setPrefix("₹ ");
    balanceSpin->setValue(0.0);
    auto *balanceHint = makeLabel(
        "How much money is in this account when you start tracking it? Enter 0 if you're not sure yet. "
        "You don't need to enter past transactions.", "fieldHint");
    balanceHint->setWordWrap(true);

    form->addRow("Account name", nameEdit);
    form->addRow("Where do you keep this money?", typeCombo);
    form->addRow("Starting amount", balanceSpin);
    layout->addLayout(form);
    layout->addWidget(nameHint);
    layout->addWidget(typeHint);
    layout->addWidget(balanceHint);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        const QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(&dialog, "Account name needed", "Please enter a name for this account.");
            nameEdit->setFocus();
            return;
        }
        for (const AccountRecord &account : m_accounts) {
            if (account.name.compare(name, Qt::CaseInsensitive) == 0) {
                QMessageBox::warning(&dialog, "Name already used",
                                     "You already have an account with this name. Choose a different name.");
                nameEdit->setFocus();
                nameEdit->selectAll();
                return;
            }
        }
        dialog.accept();
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    AccountRecord account;
    account.id = m_nextAccountId++;
    account.name = nameEdit->text().trimmed();
    account.type = typeCombo->currentText();
    account.balance = balanceSpin->value();
    m_accounts.append(account);
    refreshAccountsTable();
    refreshStatementAccountCombo();
    // Select the account just created so the next import step is straightforward.
    if (m_statementAccountCombo) {
        const int newAccountIndex = m_statementAccountCombo->findData(account.id);
        if (newAccountIndex >= 0)
            m_statementAccountCombo->setCurrentIndex(newAccountIndex);
    }
    showAccountMessage(QString("'%1' was added successfully.").arg(account.name), true);
}

void MainWindow::viewAccounts()
{
    refreshAccountsTable();

    if (m_accounts.isEmpty()) {
        if (m_statusLabel) m_statusLabel->setText("No accounts yet. Create your first account to get started.");
        return;
    }

    showAccountMessage(
        QString("Accounts loaded successfully. Total accounts: %1.").arg(m_accounts.size()),
        true);
}

void MainWindow::editAccount()
{
    if (!m_accountsTable || m_accountsTable->currentRow() < 0) return;
    const int row = m_accountsTable->currentRow();
    bool idOk = false;
    const int id = m_accountsTable->item(row, 0)->text().toInt(&idOk);
    if (!idOk) {
        showAccountMessage("We couldn't identify that account. Please select it again.", false);
        return;
    }
    int accountIndex = -1;
    for (int i = 0; i < m_accounts.size(); ++i) {
        if (m_accounts.at(i).id == id) { accountIndex = i; break; }
    }
    if (accountIndex < 0) {
        showAccountMessage("That account could not be found. Refresh the list and try again.", false);
        return;
    }
    const AccountRecord original = m_accounts.at(accountIndex);
    QDialog dialog(this);
    dialog.setWindowTitle("Edit account");
    dialog.setMinimumWidth(460);
    auto *layout = new QVBoxLayout(&dialog);
    auto *intro = makeLabel("Update the details for this account.", "panelHint");
    layout->addWidget(intro);
    auto *form = new QFormLayout;
    auto *nameEdit = new QLineEdit(original.name, &dialog);
    nameEdit->setPlaceholderText("e.g. SBI Savings or My Wallet");
    auto *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"Savings account", "Cash / Wallet", "Current account", "Credit card", "Other"});
    typeCombo->setCurrentText(original.type == "Savings" ? "Savings account" : original.type == "Cash" ? "Cash / Wallet" : original.type == "Current" ? "Current account" : original.type);
    auto *balanceSpin = new QDoubleSpinBox(&dialog);
    balanceSpin->setRange(0.0, 1000000000000.0);
    balanceSpin->setDecimals(2);
    balanceSpin->setPrefix("₹ ");
    balanceSpin->setValue(original.balance);
    form->addRow("Account name", nameEdit);
    form->addRow("Where do you keep this money?", typeCombo);
    form->addRow("Starting amount", balanceSpin);
    layout->addLayout(form);
    auto *hint = makeLabel("Starting amount means the balance when you began tracking this account.", "fieldHint");
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        const QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(&dialog, "Account name needed", "Please enter a name for this account.");
            nameEdit->setFocus();
            return;
        }
        for (const AccountRecord &account : m_accounts) {
            if (account.id != id && account.name.compare(name, Qt::CaseInsensitive) == 0) {
                QMessageBox::warning(&dialog, "Name already used",
                                     "You already have an account with this name. Choose a different name.");
                nameEdit->setFocus();
                nameEdit->selectAll();
                return;
            }
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted) return;
    m_accounts[accountIndex].name = nameEdit->text().trimmed();
    m_accounts[accountIndex].type = typeCombo->currentText();
    m_accounts[accountIndex].balance = balanceSpin->value();
    refreshAccountsTable();
    refreshStatementAccountCombo();
    showAccountMessage(QString("'%1' was updated successfully.").arg(m_accounts[accountIndex].name), true);
}

void MainWindow::deleteAccount()
{
    if (!m_accountsTable || m_accountsTable->currentRow() < 0) return;

    const int row = m_accountsTable->currentRow();
    bool idOk = false;
    const int id = m_accountsTable->item(row, 0)->text().toInt(&idOk);
    if (!idOk) {
        showAccountMessage("Unable to delete account: the selected account ID is invalid.", false);
        return;
    }

    int accountIndex = -1;
    for (int i = 0; i < m_accounts.size(); ++i) {
        if (m_accounts.at(i).id == id) {
            accountIndex = i;
            break;
        }
    }
    if (accountIndex < 0) {
        showAccountMessage("Unable to delete account: the selected account no longer exists.",
                           false);
        return;
    }

    const QString name = m_accounts.at(accountIndex).name;
    const auto answer = QMessageBox::question(
        this, "Confirm deletion",
        QString("Delete '%1'? This action cannot be undone.").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    m_accounts.removeAt(accountIndex);
    refreshAccountsTable();
    refreshStatementAccountCombo();
    showAccountMessage(QString("Account '%1' deleted successfully.").arg(name), true);
}

QWidget *MainWindow::buildImportStatementsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    auto *intro = new QFrame;
    intro->setObjectName("contentPanel");
    auto *introLayout = new QVBoxLayout(intro);
    introLayout->setContentsMargins(20, 18, 20, 18);
    introLayout->setSpacing(6);
    introLayout->addWidget(makeLabel("Import a bank statement", "panelTitle"));
    auto *introText = makeLabel(
        "Choose a CSV statement to preview transactions before importing. "
        "Review any parsing issues or possible duplicates, then confirm the import.",
        "panelHint");
    introText->setWordWrap(true);
    introLayout->addWidget(introText);
    layout->addWidget(intro);

    auto *accountForm = new QFormLayout;
    m_statementAccountCombo = new QComboBox;
    m_statementAccountCombo->setObjectName("statementAccountCombo");
    accountForm->addRow("Import transactions into account", m_statementAccountCombo);
    layout->addLayout(accountForm);
    m_statementAccountNotice = makeLabel(
        "Create an account first, then select it here before choosing a statement.", "panelHint");
    m_statementAccountNotice->setWordWrap(true);
    layout->addWidget(m_statementAccountNotice);

    auto *fileRow = new QHBoxLayout;
    m_statementPathLabel = makeLabel("No statement selected", "panelHint");
    m_statementPathLabel->setWordWrap(true);
    m_statementPathLabel->setMinimumWidth(200);
    m_chooseStatementButton = new QPushButton("Choose CSV statement");
    m_chooseStatementButton->setObjectName("primaryActionButton");
    m_chooseStatementButton->setCursor(Qt::PointingHandCursor);
    fileRow->addWidget(m_statementPathLabel, 1);
    fileRow->addWidget(m_chooseStatementButton);
    layout->addLayout(fileRow);

    auto *formatHint = makeLabel(
        "Supported format: CSV with columns for Date, Description and Amount. "
        "Example: 2026-10-20,Groceries,-450.00. Positive amounts are income; negative amounts are expenses. "
        "A Type column is optional. PDF and Excel statements are not supported by this prototype.",
        "panelHint");
    formatHint->setWordWrap(true);
    layout->addWidget(formatHint);

    auto *statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);
    m_statementDetectedLabel = makeLabel("Transactions detected: 0", "panelTitle");
    m_statementErrorLabel = makeLabel("Parsing errors: 0", "panelTitle");
    m_statementDuplicateLabel = makeLabel("Possible duplicates: 0", "panelTitle");
    for (QLabel *label : {m_statementDetectedLabel, m_statementErrorLabel, m_statementDuplicateLabel}) {
        auto *card = new QFrame;
        card->setObjectName("metricCard");
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->addWidget(label);
        statsRow->addWidget(card, 1);
    }
    layout->addLayout(statsRow);

    auto *reviewPanel = new QFrame;
    reviewPanel->setObjectName("contentPanel");
    auto *reviewLayout = new QVBoxLayout(reviewPanel);
    reviewLayout->setContentsMargins(16, 16, 16, 16);
    reviewLayout->setSpacing(8);
    reviewLayout->addWidget(makeLabel("Review transactions", "panelTitle"));
    m_statementTable = new QTableWidget(0, 5);
    m_statementTable->setObjectName("accountsTable");
    m_statementTable->setHorizontalHeaderLabels(
        {"Date", "Description", "Amount", "Review status", "Details"});
    m_statementTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_statementTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_statementTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_statementTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_statementTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_statementTable->verticalHeader()->setVisible(false);
    m_statementTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_statementTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_statementTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statementTable->setAlternatingRowColors(true);
    m_statementTable->setMinimumHeight(230);
    reviewLayout->addWidget(m_statementTable);
    layout->addWidget(reviewPanel, 1);

    m_statementErrorDetails = makeLabel(
        "Parsing errors will be listed here after you choose a file.", "panelHint");
    m_statementErrorDetails->setWordWrap(true);
    layout->addWidget(m_statementErrorDetails);

    auto *actions = new QHBoxLayout;
    m_confirmImportButton = new QPushButton("Confirm import");
    m_confirmImportButton->setObjectName("primaryActionButton");
    m_confirmImportButton->setCursor(Qt::PointingHandCursor);
    m_confirmImportButton->setEnabled(false);
    auto *clearButton = new QPushButton("Clear selection");
    clearButton->setObjectName("secondaryActionButton");
    clearButton->setCursor(Qt::PointingHandCursor);
    actions->addWidget(m_confirmImportButton);
    actions->addWidget(clearButton);
    actions->addStretch();
    layout->addLayout(actions);

    auto *notice = makeLabel(
        "Current prototype: imported rows are linked to the selected account and duplicate checks include rows "
        "imported earlier in this app session. Account and transaction data are still in memory only; closing "
        "the app clears them. Permanent SQLite saving through the transaction repository is not connected yet.",
        "panelHint");
    notice->setWordWrap(true);
    layout->addWidget(notice);

    connect(m_chooseStatementButton, &QPushButton::clicked, this, &MainWindow::chooseStatementFile);
    connect(m_confirmImportButton, &QPushButton::clicked, this, &MainWindow::confirmStatementImport);
    connect(m_statementAccountCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) {
        if (!m_statementPath.isEmpty())
            clearStatementPreview("The selected account changed. Choose the statement again to preview it for this account.");
    });
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        clearStatementPreview("No statement selected. Choose a CSV statement to begin.");
    });

    refreshStatementAccountCombo();
    return page;
}

void MainWindow::refreshStatementAccountCombo()
{
    if (!m_statementAccountCombo)
        return;

    const bool hadPreviousSelection = m_statementAccountCombo->currentIndex() > 0;
    const int previousAccountId = hadPreviousSelection
                                      ? m_statementAccountCombo->currentData().toInt() : -1;
    const QSignalBlocker blocker(m_statementAccountCombo);
    m_statementAccountCombo->clear();
    m_statementAccountCombo->addItem("Select an account...", -1);
    for (const AccountRecord &account : m_accounts)
        m_statementAccountCombo->addItem(account.name, account.id);

    int selectedIndex = 0; // Explicit selection is required; index 0 is the placeholder.
    if (hadPreviousSelection) {
        for (int i = 1; i < m_statementAccountCombo->count(); ++i) {
            if (m_statementAccountCombo->itemData(i).toInt() == previousAccountId) {
                selectedIndex = i;
                break;
            }
        }
    }
    m_statementAccountCombo->setCurrentIndex(selectedIndex);

    const bool hasAccounts = !m_accounts.isEmpty();
    const bool hasSelectedAccount = m_statementAccountCombo->currentIndex() > 0;
    m_statementAccountCombo->setEnabled(hasAccounts);
    // Keep the button clickable so a first-time user gets a clear explanation
    // instead of assuming the file picker is broken. chooseStatementFile()
    // validates that an account has been selected before opening the picker.
    if (m_chooseStatementButton) {
        m_chooseStatementButton->setEnabled(true);
        m_chooseStatementButton->setToolTip(
            hasSelectedAccount
                ? "Choose a CSV statement for the selected account."
                : "Select an account first. If none exists, create one in Accounts.");
    }
    if (m_confirmImportButton)
        m_confirmImportButton->setEnabled(hasSelectedAccount && !m_statementRows.isEmpty());
    if (m_statementAccountNotice) {
        m_statementAccountNotice->setText(hasAccounts
            ? "Every imported transaction will be linked to the selected account. If this account is wrong, choose the correct one before selecting the file."
            : "No accounts exist yet. Open Accounts and create a bank account or cash wallet first. Statement import is disabled until an account exists.");
    }

    // If the previously selected account was deleted, discard its preview rather than
    // accidentally importing those rows into a different account.
    if (!m_statementPath.isEmpty() && hadPreviousSelection && selectedIndex == 0)
        clearStatementPreview("The account used for this preview no longer exists. Select an existing account and choose the statement again.");
}

void MainWindow::clearStatementPreview(const QString &message)
{
    m_statementPath.clear();
    m_statementRows.clear();
    m_statementAccountId = -1;
    m_statementInvalidRowCount = 0;
    if (m_statementPathLabel) m_statementPathLabel->setText(message);
    if (m_statementTable) m_statementTable->setRowCount(0);
    if (m_statementDetectedLabel) m_statementDetectedLabel->setText("Transactions detected: 0");
    if (m_statementErrorLabel) m_statementErrorLabel->setText("Parsing errors: 0");
    if (m_statementDuplicateLabel) m_statementDuplicateLabel->setText("Possible duplicates: 0");
    if (m_statementErrorDetails) m_statementErrorDetails->setText("Choose a CSV statement to preview transactions and parsing issues.");
    if (m_confirmImportButton) m_confirmImportButton->setEnabled(false);
}

QStringList MainWindow::splitCsvLine(const QString &line) const
{
    QStringList fields;
    QString field;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line.at(i + 1) == '"') {
                field += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            fields << field.trimmed();
            field.clear();
        } else {
            field += ch;
        }
    }
    fields << field.trimmed();
    return fields;
}

void MainWindow::chooseStatementFile()
{
    if (m_accounts.isEmpty()) {
        QMessageBox::information(this, "Create an account first",
            "No account exists in this running session. Go to Accounts and create the account these transactions belong to. "
            "Accounts are currently temporary and are cleared when the application closes.");
        return;
    }
    if (!m_statementAccountCombo || m_statementAccountCombo->currentIndex() <= 0) {
        QMessageBox::information(this, "Select an account",
            "An account already exists, but no account is selected for this import. "
            "Choose it from the account dropdown, then click Choose CSV statement again.");
        return;
    }

    const int selectedAccountId = m_statementAccountCombo->currentData().toInt();
    bool accountExists = false;
    for (const AccountRecord &account : m_accounts) {
        if (account.id == selectedAccountId) { accountExists = true; break; }
    }
    if (!accountExists) {
        QMessageBox::warning(this, "Account unavailable", "Choose an existing account before importing.");
        refreshStatementAccountCombo();
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this, "Select a bank statement", QString(),
        "CSV statement (*.csv);;All files (*)");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Cannot open statement",
                             "The selected file could not be opened. Check its permissions and try again.");
        return;
    }

    m_statementPath = filePath;
    m_statementAccountId = selectedAccountId;
    m_statementRows.clear();
    m_statementInvalidRowCount = 0;
    QStringList errors;
    QSet<QString> seenKeys;
    int duplicateCount = 0;
    int lineNumber = 0;
    bool headerFound = false;
    int dateColumn = -1, descriptionColumn = -1, amountColumn = -1, typeColumn = -1;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString rawLine = stream.readLine();
        ++lineNumber;
        if (rawLine.trimmed().isEmpty())
            continue;

        QStringList fields = splitCsvLine(rawLine);
        if (!headerFound) {
            for (int i = 0; i < fields.size(); ++i) {
                const QString heading = fields.at(i).trimmed().toLower();
                if (heading == "date" || heading == "transaction date") dateColumn = i;
                else if (heading == "description" || heading == "details" || heading == "narration" || heading == "merchant") descriptionColumn = i;
                else if (heading == "amount" || heading == "transaction amount") amountColumn = i;
                else if (heading == "type" || heading == "transaction type") typeColumn = i;
            }
            if (dateColumn < 0 || descriptionColumn < 0 || amountColumn < 0) {
                errors << QString("Line %1: header must include Date, Description and Amount columns.").arg(lineNumber);
                break;
            }
            headerFound = true;
            continue;
        }

        const int requiredMax = qMax(dateColumn, qMax(descriptionColumn, amountColumn));
        if (fields.size() <= requiredMax) {
            ++m_statementInvalidRowCount;
            errors << QString("Line %1: not enough columns; row skipped.").arg(lineNumber);
            continue;
        }

        StatementRecord record;
        record.date = fields.at(dateColumn).trimmed();
        record.description = fields.at(descriptionColumn).trimmed();
        QString amountText = fields.at(amountColumn).trimmed();
        amountText.remove(QRegularExpression("[₹,$ ]"));
        bool amountOk = false;
        record.amount = amountText.toDouble(&amountOk);
        record.type = (typeColumn >= 0 && typeColumn < fields.size())
                          ? fields.at(typeColumn).trimmed() : QString();

        const QDate parsedDate = QDate::fromString(record.date, Qt::ISODate);
        if (!parsedDate.isValid()) {
            ++m_statementInvalidRowCount;
            errors << QString("Line %1: invalid date '%2'; row skipped (use YYYY-MM-DD).").arg(lineNumber).arg(record.date);
            continue;
        }
        if (record.description.isEmpty()) {
            ++m_statementInvalidRowCount;
            errors << QString("Line %1: description is empty; row skipped.").arg(lineNumber);
            continue;
        }
        if (!amountOk || !qIsFinite(record.amount)) {
            ++m_statementInvalidRowCount;
            errors << QString("Line %1: invalid amount '%2'; row skipped.").arg(lineNumber).arg(fields.at(amountColumn));
            continue;
        }

        const QString key = record.date + "|" + record.description.simplified().toLower()
                            + "|" + QString::number(record.amount, 'f', 2);
        record.duplicate = seenKeys.contains(key);
        if (!record.duplicate) {
            for (const ImportedTransactionRecord &existing : m_importedTransactions) {
                if (existing.accountId == selectedAccountId
                    && existing.date == record.date
                    && existing.description.simplified().compare(record.description.simplified(), Qt::CaseInsensitive) == 0
                    && qAbs(existing.amount - record.amount) < 0.005) {
                    record.duplicate = true;
                    break;
                }
            }
        }
        if (record.duplicate)
            ++duplicateCount;
        else
            seenKeys.insert(key);
        record.lineNumber = lineNumber;
        record.valid = true;
        m_statementRows.append(record);
    }
    file.close();

    if (!headerFound && errors.isEmpty())
        errors << "The selected file is empty or has no header row.";

    if (m_statementPathLabel)
        m_statementPathLabel->setText(QFileInfo(filePath).fileName());
    if (m_statementDetectedLabel)
        m_statementDetectedLabel->setText(QString("Transactions detected: %1").arg(m_statementRows.size()));
    if (m_statementErrorLabel)
        m_statementErrorLabel->setText(QString("Parsing errors: %1").arg(errors.size()));
    if (m_statementDuplicateLabel)
        m_statementDuplicateLabel->setText(QString("Possible duplicates: %1").arg(duplicateCount));
    if (m_statementErrorDetails)
        m_statementErrorDetails->setText(errors.isEmpty()
            ? "No parsing errors found."
            : "Parsing issues:\n• " + errors.mid(0, 8).join("\n• ")
              + (errors.size() > 8 ? QString("\n• ...and %1 more.").arg(errors.size() - 8) : QString()));

    if (m_statementTable) {
        m_statementTable->setRowCount(0);
        for (const StatementRecord &record : m_statementRows) {
            const int row = m_statementTable->rowCount();
            m_statementTable->insertRow(row);
            m_statementTable->setItem(row, 0, new QTableWidgetItem(record.date));
            m_statementTable->setItem(row, 1, new QTableWidgetItem(record.description));
            auto *amountItem = new QTableWidgetItem(QString("₹ %1").arg(record.amount, 0, 'f', 2));
            amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_statementTable->setItem(row, 2, amountItem);
            bool matchesPriorImport = false;
            if (record.duplicate) {
                for (const ImportedTransactionRecord &existing : m_importedTransactions) {
                    if (existing.accountId == selectedAccountId && existing.date == record.date
                        && existing.description.simplified().compare(record.description.simplified(), Qt::CaseInsensitive) == 0
                        && qAbs(existing.amount - record.amount) < 0.005) {
                        matchesPriorImport = true;
                        break;
                    }
                }
            }
            m_statementTable->setItem(row, 3, new QTableWidgetItem(record.duplicate ? "Possible duplicate" : "Ready"));
            m_statementTable->setItem(row, 4, new QTableWidgetItem(record.duplicate
                ? (matchesPriorImport ? "Matches a transaction already imported to this account" : "Repeated date, description and amount in this file")
                : QString("CSV line %1 · Account ID %2").arg(record.lineNumber).arg(selectedAccountId)));
        }
    }

    if (m_confirmImportButton)
        m_confirmImportButton->setEnabled(headerFound && !m_statementRows.isEmpty()
                                          && m_statementAccountId == selectedAccountId);

    if (!headerFound)
        QMessageBox::warning(this, "Statement format not recognised",
                             "This file needs a CSV header with Date, Description and Amount columns.");
}

void MainWindow::confirmStatementImport()
{
    if (m_accounts.isEmpty() || !m_statementAccountCombo || m_statementAccountCombo->currentIndex() <= 0) {
        QMessageBox::warning(this, "Create an account first",
            "No account is available. Create an account before importing a statement.");
        return;
    }
    if (m_statementRows.isEmpty() || m_statementPath.isEmpty()) {
        QMessageBox::warning(this, "Nothing to import", "Choose a valid CSV statement first.");
        return;
    }

    const int selectedAccountId = m_statementAccountCombo->currentData().toInt();
    bool accountExists = false;
    QString accountName;
    for (const AccountRecord &account : m_accounts) {
        if (account.id == selectedAccountId) {
            accountExists = true;
            accountName = account.name;
            break;
        }
    }
    if (!accountExists || selectedAccountId != m_statementAccountId) {
        QMessageBox::warning(this, "Account changed",
            "The account selected for this preview is no longer the same. Choose the statement again to review it for the selected account.");
        clearStatementPreview("Account changed. Choose the statement again to preview it.");
        return;
    }

    const int duplicateCount = static_cast<int>(std::count_if(
        m_statementRows.cbegin(), m_statementRows.cend(),
        [](const StatementRecord &r) { return r.duplicate; }));
    const auto answer = QMessageBox::question(
        this, "Confirm statement import",
        QString("Import %1 non-duplicate transaction(s) into '%2'?\n\n"
                "Possible duplicates to skip: %3\nRows with parsing errors to skip: %4\n\n"
                "Please confirm only if the selected account and transaction preview are correct.")
            .arg(m_statementRows.size() - duplicateCount)
            .arg(accountName)
            .arg(duplicateCount)
            .arg(m_statementInvalidRowCount),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    int imported = 0;
    int skippedDuplicates = 0;
    for (const StatementRecord &record : m_statementRows) {
        if (record.duplicate) {
            ++skippedDuplicates;
            continue;
        }
        ImportedTransactionRecord transaction;
        transaction.accountId = selectedAccountId;
        transaction.date = record.date;
        transaction.description = record.description;
        transaction.amount = record.amount;
        m_importedTransactions.append(transaction);
        ++imported;
    }

    const int skippedTotal = skippedDuplicates + m_statementInvalidRowCount;
    QMessageBox::information(
        this, "Import summary",
        QString("Statement import completed for '%1'.\n\n"
                "Transactions linked to this account in the current session: %2\n"
                "Skipped possible duplicates: %3\n"
                "Skipped rows with parsing errors: %4\n"
                "Total skipped: %5\n\n"
                "Important: these imported transactions are currently held in memory only. They will be lost when the app closes; permanent SQLite saving still needs to be connected.")
            .arg(accountName).arg(imported).arg(skippedDuplicates)
            .arg(m_statementInvalidRowCount).arg(skippedTotal));

    if (m_statusLabel)
        m_statusLabel->setText(QString("Imported %1 transaction(s) to %2 in this session; %3 skipped.")
                                   .arg(imported).arg(accountName).arg(skippedTotal));

    // Disable a second confirmation of this same preview, avoiding double-import during this session.
    if (m_confirmImportButton)
        m_confirmImportButton->setEnabled(false);
}

QWidget *MainWindow::buildMetricCard(const QString &caption,
                                     const QString &value,
                                     const QString &detail)
{
    auto *card = new QFrame;
    card->setObjectName("metricCard");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(9);

    auto *captionLabel = makeLabel(caption, "metricCaption");
    auto *valueLabel = makeLabel(value, "metricValue");
    auto *detailLabel = makeLabel(detail, "metricDetail");
    detailLabel->setWordWrap(true);

    layout->addWidget(captionLabel);
    layout->addWidget(valueLabel);
    layout->addStretch();
    layout->addWidget(detailLabel);
    return card;
}


QWidget *MainWindow::buildTransactionsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(12);

    auto *intro = new QFrame;
    intro->setObjectName("contentPanel");
    auto *introLayout = new QVBoxLayout(intro);
    introLayout->addWidget(makeLabel("Transaction management", "panelTitle"));
    auto *hint = makeLabel("Record income and expenses against an account, then search and review your activity.", "panelHint");
    hint->setWordWrap(true);
    introLayout->addWidget(hint);
    layout->addWidget(intro);

    auto *actions = new QHBoxLayout;
    auto *expenseButton = new QPushButton("＋ Add expense");
    auto *incomeButton = new QPushButton("＋ Add income");
    expenseButton->setObjectName("primaryActionButton");
    incomeButton->setObjectName("secondaryActionButton");
    m_editTransactionButton = new QPushButton("Edit selected");
    m_deleteTransactionButton = new QPushButton("Delete selected");
    m_editTransactionButton->setObjectName("secondaryActionButton");
    m_deleteTransactionButton->setObjectName("dangerActionButton");
    m_editTransactionButton->setEnabled(false);
    m_deleteTransactionButton->setEnabled(false);
    actions->addWidget(expenseButton);
    actions->addWidget(incomeButton);
    actions->addWidget(m_editTransactionButton);
    actions->addWidget(m_deleteTransactionButton);
    actions->addStretch();
    layout->addLayout(actions);

    auto *filters = new QHBoxLayout;
    m_transactionSearch = new QLineEdit;
    m_transactionSearch->setPlaceholderText("Search description or category…");
    m_transactionTypeFilter = new QComboBox;
    m_transactionTypeFilter->addItems({"All types", "Expense", "Income"});
    filters->addWidget(m_transactionSearch, 2);
    filters->addWidget(m_transactionTypeFilter, 1);
    layout->addLayout(filters);

    m_transactionsTable = new QTableWidget(0, 6);
    m_transactionsTable->setObjectName("accountsTable");
    m_transactionsTable->setHorizontalHeaderLabels({"Date", "Description", "Category", "Type", "Amount", "Account"});
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_transactionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_transactionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_transactionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_transactionsTable->setAlternatingRowColors(true);
    layout->addWidget(m_transactionsTable, 1);

    connect(expenseButton, &QPushButton::clicked, this, [this]() { addTransaction(false); });
    connect(incomeButton, &QPushButton::clicked, this, [this]() { addTransaction(true); });
    connect(m_editTransactionButton, &QPushButton::clicked, this, &MainWindow::editTransaction);
    connect(m_deleteTransactionButton, &QPushButton::clicked, this, &MainWindow::deleteTransaction);
    connect(m_transactionsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool selected = m_transactionsTable && m_transactionsTable->currentRow() >= 0;
        m_editTransactionButton->setEnabled(selected);
        m_deleteTransactionButton->setEnabled(selected);
    });
    auto refreshFilter = [this]() { refreshTransactionsTable(); };
    connect(m_transactionSearch, &QLineEdit::textChanged, this, refreshFilter);
    connect(m_transactionTypeFilter, &QComboBox::currentTextChanged, this, refreshFilter);
    refreshTransactionsTable();
    return page;
}

void MainWindow::refreshTransactionsTable()
{
    if (!m_transactionsTable) return;
    m_transactionsTable->setRowCount(0);
    const QString search = m_transactionSearch ? m_transactionSearch->text().trimmed().toLower() : QString();
    const QString typeFilter = m_transactionTypeFilter ? m_transactionTypeFilter->currentText() : "All types";
    for (const auto &t : m_transactions) {
        if (typeFilter != "All types" && t.type != typeFilter) continue;
        if (!search.isEmpty() && !t.description.toLower().contains(search) && !t.category.toLower().contains(search)) continue;
        QString accountName = "Unknown account";
        for (const auto &a : m_accounts) if (a.id == t.accountId) accountName = a.name;
        const int row = m_transactionsTable->rowCount();
        m_transactionsTable->insertRow(row);
        m_transactionsTable->setItem(row, 0, new QTableWidgetItem(t.date));
        m_transactionsTable->setItem(row, 1, new QTableWidgetItem(t.description));
        m_transactionsTable->setItem(row, 2, new QTableWidgetItem(t.category));
        m_transactionsTable->setItem(row, 3, new QTableWidgetItem(t.type));
        auto *amount = new QTableWidgetItem(QString("%1 ₹ %2").arg(t.type == "Income" ? "+" : "−").arg(t.amount, 0, 'f', 2));
        amount->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_transactionsTable->setItem(row, 4, amount);
        m_transactionsTable->setItem(row, 5, new QTableWidgetItem(accountName));
        m_transactionsTable->item(row, 0)->setData(Qt::UserRole, t.id);
    }
    if (m_editTransactionButton) m_editTransactionButton->setEnabled(false);
    if (m_deleteTransactionButton) m_deleteTransactionButton->setEnabled(false);
}

void MainWindow::addTransaction(bool income)
{
    if (m_accounts.isEmpty()) {
        QMessageBox::information(this, "Create an account first", "Create an account before recording a transaction.");
        navigateTo(2);
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(income ? "Add income" : "Add expense");
    dialog.setMinimumWidth(420);
    auto *form = new QFormLayout(&dialog);
    auto *date = new QDateEdit(QDate::currentDate());
    date->setCalendarPopup(true);
    auto *description = new QLineEdit;
    description->setPlaceholderText(income ? "e.g. Monthly salary" : "e.g. Groceries");
    auto *category = new QComboBox;
    category->setEditable(true);
    category->addItems(income ? QStringList{"Salary", "Allowance", "Interest", "Other"} :
                                 QStringList{"Food & groceries", "Transport", "Housing", "Entertainment", "Health", "Education", "Other"});
    auto *amount = new QDoubleSpinBox;
    amount->setRange(0.01, 999999999.99);
    amount->setDecimals(2);
    amount->setPrefix("₹ ");
    auto *account = new QComboBox;
    for (const auto &a : m_accounts) account->addItem(a.name, a.id);
    form->addRow("Date", date);
    form->addRow("Description", description);
    form->addRow("Category", category);
    form->addRow("Amount", amount);
    form->addRow("Account", account);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    if (description->text().trimmed().isEmpty() || category->currentText().trimmed().isEmpty() || amount->value() <= 0) {
        QMessageBox::warning(this, "Check transaction details", "Enter a description, category and amount greater than zero.");
        return;
    }
    TransactionRecord t;
    t.id = m_nextTransactionId++;
    t.accountId = account->currentData().toInt();
    t.date = date->date().toString(Qt::ISODate);
    t.description = description->text().trimmed();
    t.category = category->currentText().trimmed();
    t.type = income ? "Income" : "Expense";
    t.amount = amount->value();
    m_transactions.append(t);
    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();
    QMessageBox::information(this, "Transaction saved", "The transaction was added to this application session.");
}

void MainWindow::editTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) return;
    const int id = m_transactionsTable->item(row, 0)->data(Qt::UserRole).toInt();
    int index = -1;
    for (int i = 0; i < m_transactions.size(); ++i) if (m_transactions[i].id == id) index = i;
    if (index < 0) return;
    auto &t = m_transactions[index];
    QDialog dialog(this); dialog.setWindowTitle("Edit transaction"); dialog.setMinimumWidth(420);
    auto *form = new QFormLayout(&dialog);
    auto *date = new QDateEdit(QDate::fromString(t.date, Qt::ISODate)); date->setCalendarPopup(true);
    auto *description = new QLineEdit(t.description);
    auto *category = new QLineEdit(t.category);
    auto *amount = new QDoubleSpinBox; amount->setRange(0.01, 999999999.99); amount->setDecimals(2); amount->setPrefix("₹ "); amount->setValue(t.amount);
    auto *account = new QComboBox;
    for (const auto &a : m_accounts) account->addItem(a.name, a.id);
    account->setCurrentIndex(qMax(0, account->findData(t.accountId)));
    form->addRow("Date", date); form->addRow("Description", description); form->addRow("Category", category); form->addRow("Amount", amount); form->addRow("Account", account);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel); form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    if (description->text().trimmed().isEmpty() || category->text().trimmed().isEmpty()) { QMessageBox::warning(this, "Invalid transaction", "Description and category are required."); return; }
    t.date = date->date().toString(Qt::ISODate); t.description = description->text().trimmed(); t.category = category->text().trimmed(); t.amount = amount->value(); t.accountId = account->currentData().toInt();
    refreshTransactionsTable(); refreshBudgetsTable(); refreshDashboard();
    QMessageBox::information(this, "Transaction updated", "The transaction was updated for this session.");
}

void MainWindow::deleteTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) return;
    const int id = m_transactionsTable->item(row, 0)->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "Delete transaction", "Delete the selected transaction?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    for (int i = 0; i < m_transactions.size(); ++i) if (m_transactions[i].id == id) { m_transactions.remove(i); break; }
    refreshTransactionsTable(); refreshBudgetsTable(); refreshDashboard();
}

QWidget *MainWindow::buildBudgetsPage()
{
    auto *page = new QWidget; auto *layout = new QVBoxLayout(page); layout->setSpacing(12);
    auto *intro = new QFrame; intro->setObjectName("contentPanel"); auto *il = new QVBoxLayout(intro);
    il->addWidget(makeLabel("Budget management", "panelTitle"));
    il->addWidget(makeLabel("Set a monthly category limit and track actual spending.", "panelHint"));
    layout->addWidget(intro);
    auto *actions = new QHBoxLayout;
    auto *create = new QPushButton("＋ Create budget"); create->setObjectName("primaryActionButton");
    m_editBudgetButton = new QPushButton("Edit selected"); m_editBudgetButton->setObjectName("secondaryActionButton"); m_editBudgetButton->setEnabled(false);
    m_deleteBudgetButton = new QPushButton("Delete selected"); m_deleteBudgetButton->setObjectName("dangerActionButton"); m_deleteBudgetButton->setEnabled(false);
    actions->addWidget(create); actions->addWidget(m_editBudgetButton); actions->addWidget(m_deleteBudgetButton); actions->addStretch(); layout->addLayout(actions);
    m_budgetsTable = new QTableWidget(0, 6); m_budgetsTable->setObjectName("accountsTable");
    m_budgetsTable->setHorizontalHeaderLabels({"Month", "Category", "Budget limit", "Spent", "Remaining", "Status"});
    m_budgetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_budgetsTable->setSelectionBehavior(QAbstractItemView::SelectRows); m_budgetsTable->setSelectionMode(QAbstractItemView::SingleSelection); m_budgetsTable->setEditTriggers(QAbstractItemView::NoEditTriggers); m_budgetsTable->setAlternatingRowColors(true);
    layout->addWidget(m_budgetsTable, 1);
    connect(create, &QPushButton::clicked, this, &MainWindow::addBudget);
    connect(m_editBudgetButton, &QPushButton::clicked, this, &MainWindow::editBudget);
    connect(m_deleteBudgetButton, &QPushButton::clicked, this, &MainWindow::deleteBudget);
    connect(m_budgetsTable, &QTableWidget::itemSelectionChanged, this, [this]() { bool selected = m_budgetsTable->currentRow() >= 0; m_editBudgetButton->setEnabled(selected); m_deleteBudgetButton->setEnabled(selected); });
    refreshBudgetsTable(); return page;
}

void MainWindow::refreshBudgetsTable()
{
    if (!m_budgetsTable) return;
    m_budgetsTable->setRowCount(0);
    const QString month = QDate::currentDate().toString("yyyy-MM");
    for (const auto &b : m_budgets) {
        double spent = 0;
        for (const auto &t : m_transactions) if (t.type == "Expense" && t.category.compare(b.category, Qt::CaseInsensitive) == 0 && t.date.left(7) == b.month) spent += t.amount;
        const int row = m_budgetsTable->rowCount(); m_budgetsTable->insertRow(row);
        m_budgetsTable->setItem(row, 0, new QTableWidgetItem(b.month));
        m_budgetsTable->setItem(row, 1, new QTableWidgetItem(b.category));
        m_budgetsTable->setItem(row, 2, new QTableWidgetItem(QString("₹ %1").arg(b.limit, 0, 'f', 2)));
        m_budgetsTable->setItem(row, 3, new QTableWidgetItem(QString("₹ %1").arg(spent, 0, 'f', 2)));
        m_budgetsTable->setItem(row, 4, new QTableWidgetItem(QString("₹ %1").arg(b.limit - spent, 0, 'f', 2)));
        m_budgetsTable->setItem(row, 5, new QTableWidgetItem(spent > b.limit ? "OVER BUDGET" : (spent >= b.limit * 0.8 ? "Near limit" : "On track")));
        m_budgetsTable->item(row, 0)->setData(Qt::UserRole, b.id);
        if (spent > b.limit) for (int c = 0; c < 6; ++c) m_budgetsTable->item(row,c)->setBackground(QColor("#fff0f0"));
    }
    if (m_editBudgetButton) m_editBudgetButton->setEnabled(false);
    if (m_deleteBudgetButton) m_deleteBudgetButton->setEnabled(false);
}

void MainWindow::addBudget()
{
    QDialog d(this); d.setWindowTitle("Create budget"); auto *f = new QFormLayout(&d);
    auto *category = new QComboBox; category->setEditable(true); category->addItems({"Food & groceries", "Transport", "Housing", "Entertainment", "Health", "Education", "Utilities", "Other"});
    auto *limit = new QDoubleSpinBox; limit->setRange(0.01, 999999999.99); limit->setDecimals(2); limit->setPrefix("₹ ");
    auto *month = new QDateEdit(QDate::currentDate()); month->setDisplayFormat("MMMM yyyy"); month->setCalendarPopup(true);
    f->addRow("Category", category); f->addRow("Monthly limit", limit); f->addRow("Month", month);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel); f->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    if (d.exec()!=QDialog::Accepted) return;
    if (category->currentText().trimmed().isEmpty() || limit->value() <= 0) { QMessageBox::warning(this,"Invalid budget","Enter a category and a positive limit."); return; }
    for (const auto &b : m_budgets) if (b.category.compare(category->currentText().trimmed(),Qt::CaseInsensitive)==0 && b.month==month->date().toString("yyyy-MM")) { QMessageBox::warning(this,"Budget exists","A budget for this category and month already exists."); return; }
    m_budgets.append({m_nextBudgetId++, category->currentText().trimmed(), limit->value(), month->date().toString("yyyy-MM")});
    refreshBudgetsTable(); refreshDashboard(); QMessageBox::information(this,"Budget created","Budget created for this session.");
}

void MainWindow::editBudget()
{
    int row=m_budgetsTable?m_budgetsTable->currentRow():-1; if(row<0)return;
    int id=m_budgetsTable->item(row,0)->data(Qt::UserRole).toInt(); int idx=-1;
    for(int i=0;i<m_budgets.size();++i)if(m_budgets[i].id==id)idx=i; if(idx<0)return;
    auto &b=m_budgets[idx]; QDialog d(this); d.setWindowTitle("Edit budget"); auto *f=new QFormLayout(&d);
    auto *category=new QLineEdit(b.category); auto *limit=new QDoubleSpinBox; limit->setRange(0.01,999999999.99); limit->setDecimals(2); limit->setPrefix("₹ "); limit->setValue(b.limit);
    auto *month=new QDateEdit(QDate::fromString(b.month+"-01",Qt::ISODate)); month->setDisplayFormat("MMMM yyyy"); month->setCalendarPopup(true);
    f->addRow("Category",category); f->addRow("Monthly limit",limit); f->addRow("Month",month);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel); f->addRow(buttons); connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    if(d.exec()!=QDialog::Accepted)return; if(category->text().trimmed().isEmpty()){QMessageBox::warning(this,"Invalid budget","Category is required.");return;}
    b.category=category->text().trimmed(); b.limit=limit->value(); b.month=month->date().toString("yyyy-MM"); refreshBudgetsTable(); refreshDashboard(); QMessageBox::information(this,"Budget updated","Budget updated for this session.");
}

void MainWindow::deleteBudget()
{
    int row=m_budgetsTable?m_budgetsTable->currentRow():-1; if(row<0)return;
    int id=m_budgetsTable->item(row,0)->data(Qt::UserRole).toInt();
    if(QMessageBox::question(this,"Delete budget","Delete this budget?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes)return;
    for(int i=0;i<m_budgets.size();++i)if(m_budgets[i].id==id){m_budgets.remove(i);break;}
    refreshBudgetsTable(); refreshDashboard();
}

QWidget *MainWindow::buildReportsPage()
{
    auto *page=new QWidget; auto *layout=new QVBoxLayout(page); layout->setSpacing(12);
    auto *panel=new QFrame; panel->setObjectName("contentPanel"); auto *f=new QFormLayout(panel);
    m_reportTypeCombo=new QComboBox; m_reportTypeCombo->addItems({"Income and expenses","Category spending","Budget status","Account balances"});
    m_reportPeriodCombo=new QComboBox; m_reportPeriodCombo->addItems({"This month","Last month","All time"});
    auto *run=new QPushButton("Generate report"); run->setObjectName("primaryActionButton");
    f->addRow("Report type",m_reportTypeCombo); f->addRow("Period",m_reportPeriodCombo); f->addRow(run);
    layout->addWidget(panel);
    m_reportSummary=makeLabel("Choose a report type and period, then generate a report.","panelHint"); layout->addWidget(m_reportSummary);
    m_reportTable=new QTableWidget(0,3); m_reportTable->setObjectName("accountsTable"); m_reportTable->setHorizontalHeaderLabels({"Item","Amount","Details"}); m_reportTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); m_reportTable->setEditTriggers(QAbstractItemView::NoEditTriggers); layout->addWidget(m_reportTable,1);
    connect(run,&QPushButton::clicked,this,&MainWindow::runReport); return page;
}

void MainWindow::runReport()
{
    if(!m_reportTable)return; m_reportTable->setRowCount(0);
    const QString type=m_reportTypeCombo->currentText(), period=m_reportPeriodCombo->currentText();
    const QString thisMonth=QDate::currentDate().toString("yyyy-MM"), lastMonth=QDate::currentDate().addMonths(-1).toString("yyyy-MM");
    auto inPeriod=[&](const QString &date){return period=="All time" || (period=="This month" && date.left(7)==thisMonth) || (period=="Last month" && date.left(7)==lastMonth);};
    auto add=[&](const QString &name,double amount,const QString &detail){int r=m_reportTable->rowCount();m_reportTable->insertRow(r);m_reportTable->setItem(r,0,new QTableWidgetItem(name));m_reportTable->setItem(r,1,new QTableWidgetItem(QString("₹ %1").arg(amount,0,'f',2)));m_reportTable->setItem(r,2,new QTableWidgetItem(detail));};
    double income=0,expense=0; QMap<QString,double> categories;
    for(const auto &t:m_transactions)if(inPeriod(t.date)){if(t.type=="Income")income+=t.amount;else{expense+=t.amount;categories[t.category]+=t.amount;}}
    if(type=="Income and expenses"){add("Income",income,period);add("Expenses",expense,period);add("Net savings",income-expense,"Income minus expenses");}
    else if(type=="Category spending"){for(auto it=categories.cbegin();it!=categories.cend();++it)add(it.key(),it.value(),"Expense category");}
    else if(type=="Budget status"){for(const auto &b:m_budgets){double spent=0;for(const auto &t:m_transactions)if(t.type=="Expense"&&t.category.compare(b.category,Qt::CaseInsensitive)==0&&t.date.left(7)==b.month)spent+=t.amount;add(b.category,spent,QString("Limit ₹ %1 · %2").arg(b.limit,0,'f',2).arg(spent>b.limit?"OVER BUDGET":"On track"));}}
    else {for(const auto &a:m_accounts){double bal=a.balance;for(const auto &t:m_transactions)if(t.accountId==a.id)bal+=(t.type=="Income"?t.amount:-t.amount);add(a.name,bal,"Calculated session balance");}}
    m_reportSummary->setText(QString("%1 · %2 · based on %3 transaction(s).").arg(type,period).arg(m_transactions.size()));
}

void MainWindow::refreshDashboard()
{
    // Dashboard values are derived from current in-memory account, transaction and budget data.
    // The page is rebuilt on demand so the figures never silently remain hard-coded.
    if (!m_pages || m_pages->count() == 0) return;
    QWidget *old = m_pages->widget(0);
    QWidget *fresh = buildDashboardPage();
    m_pages->removeWidget(old);
    old->deleteLater();
    m_pages->insertWidget(0, fresh);
    if (m_pages->currentIndex() == 0) m_pages->setCurrentIndex(0);
}

QWidget *MainWindow::buildUtilitiesPage()
{
    auto *page=new QWidget; auto *layout=new QVBoxLayout(page); layout->setSpacing(12);
    auto *panel=new QFrame; panel->setObjectName("contentPanel"); auto *f=new QVBoxLayout(panel);
    f->addWidget(makeLabel("Data utilities", "panelTitle"));
    auto *desc=makeLabel("Back up the current session to a JSON file, or restore a previously created PFBM backup. Utilities are kept separate from finance calculations.", "panelHint"); desc->setWordWrap(true); f->addWidget(desc);
    auto *backup=new QPushButton("Back up data…"); backup->setObjectName("primaryActionButton");
    auto *restore=new QPushButton("Restore backup…"); restore->setObjectName("secondaryActionButton");
    auto *about=new QPushButton("About PFBM"); about->setObjectName("secondaryActionButton");
    f->addWidget(backup); f->addWidget(restore); f->addWidget(about); layout->addWidget(panel); layout->addStretch();
    connect(backup,&QPushButton::clicked,this,&MainWindow::backupData);
    connect(restore,&QPushButton::clicked,this,&MainWindow::restoreData);
    connect(about,&QPushButton::clicked,this,[this](){QMessageBox::information(this,"About PFBM","Personal Finance & Budgeting Dashboard\\nFrontend prototype\\nData currently lives in memory unless exported to a backup file.");});
    return page;
}

void MainWindow::backupData()
{
    const QString path=QFileDialog::getSaveFileName(this,"Back up PFBM data",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/pfbm-backup.json","JSON backup (*.json)");
    if(path.isEmpty())return;
    QJsonObject root; QJsonArray accounts,transactions,budgets;
    for(const auto &a:m_accounts){QJsonObject o;o["id"]=a.id;o["name"]=a.name;o["type"]=a.type;o["balance"]=a.balance;accounts.append(o);}
    for(const auto &t:m_transactions){QJsonObject o;o["id"]=t.id;o["accountId"]=t.accountId;o["date"]=t.date;o["description"]=t.description;o["category"]=t.category;o["type"]=t.type;o["amount"]=t.amount;transactions.append(o);}
    for(const auto &b:m_budgets){QJsonObject o;o["id"]=b.id;o["category"]=b.category;o["limit"]=b.limit;o["month"]=b.month;budgets.append(o);}
    root["format"]="PFBM-backup-v1";root["accounts"]=accounts;root["transactions"]=transactions;root["budgets"]=budgets;
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)){QMessageBox::warning(this,"Backup failed",file.errorString());return;}
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));if(!file.commit()){QMessageBox::warning(this,"Backup failed",file.errorString());return;}
    QMessageBox::information(this,"Backup complete","Session data backed up to:\\n"+path);
}

void MainWindow::restoreData()
{
    const QString path=QFileDialog::getOpenFileName(this,"Restore PFBM backup",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),"JSON backup (*.json)");
    if(path.isEmpty())return;
    QFile file(path);if(!file.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Restore failed",file.errorString());return;}
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!doc.isObject()||doc.object().value("format").toString()!="PFBM-backup-v1"){QMessageBox::warning(this,"Restore failed","This file is not a valid PFBM backup.");return;}
    if(QMessageBox::question(this,"Confirm restore","Restoring replaces the current in-memory accounts, transactions and budgets. Continue?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes)return;
    m_accounts.clear();m_transactions.clear();m_budgets.clear();
    for(const auto v:doc.object().value("accounts").toArray()){auto o=v.toObject();m_accounts.append({o["id"].toInt(),o["name"].toString(),o["type"].toString(),o["balance"].toDouble()});}
    for(const auto v:doc.object().value("transactions").toArray()){auto o=v.toObject();m_transactions.append({o["id"].toInt(),o["accountId"].toInt(),o["date"].toString(),o["description"].toString(),o["category"].toString(),o["type"].toString(),o["amount"].toDouble()});}
    for(const auto v:doc.object().value("budgets").toArray()){auto o=v.toObject();m_budgets.append({o["id"].toInt(),o["category"].toString(),o["limit"].toDouble(),o["month"].toString()});}
    m_nextAccountId=1;for(const auto &a:m_accounts)m_nextAccountId=qMax(m_nextAccountId,a.id+1);
    m_nextTransactionId=1;for(const auto &t:m_transactions)m_nextTransactionId=qMax(m_nextTransactionId,t.id+1);
    m_nextBudgetId=1;for(const auto &b:m_budgets)m_nextBudgetId=qMax(m_nextBudgetId,b.id+1);
    refreshAccountsTable();refreshStatementAccountCombo();refreshTransactionsTable();refreshBudgetsTable();refreshDashboard();
    QMessageBox::information(this,"Restore complete","Backup restored for this session. It will remain temporary until database persistence is added.");
}

QWidget *MainWindow::buildPlaceholderPage(const QString &title,
                                          const QString &description)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);

    auto *panel = new QFrame;
    panel->setObjectName("contentPanel");
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(24, 24, 24, 24);
    panelLayout->setSpacing(12);

    auto *titleLabel = makeLabel(title, "placeholderTitle");
    auto *descriptionLabel = makeLabel(description, "placeholderDescription");
    descriptionLabel->setWordWrap(true);

    auto *stateLabel = makeLabel("UI PLACEHOLDER", "placeholderTag");
    auto *detailLabel = makeLabel(
        "Navigation is working. This screen will be implemented when its service and data model "
        "are ready. The frontend does not access SQLite directly.");
    detailLabel->setWordWrap(true);
    detailLabel->setObjectName("panelHint");

    panelLayout->addWidget(titleLabel);
    panelLayout->addWidget(descriptionLabel);
    panelLayout->addSpacing(10);
    panelLayout->addWidget(stateLabel, 0, Qt::AlignLeft);
    panelLayout->addWidget(detailLabel);
    panelLayout->addStretch();

    layout->addWidget(panel);
    layout->addStretch();
    return page;
}

void MainWindow::navigateTo(int pageIndex)
{
    if (!m_pages || pageIndex < 0 || pageIndex >= m_pages->count())
        return;

    m_pages->setCurrentIndex(pageIndex);
    if (m_pageTitle)
        m_pageTitle->setText(navigationItems.value(pageIndex, "Dashboard"));

    if (m_statusLabel) {
        m_statusLabel->setText(
            pageIndex == 0
                ? "Frontend prototype • Demo figures only • No database connection yet"
                : "Frontend prototype • " + navigationItems.value(pageIndex)
                    + " screen • Backend integration pending");
    }

    if (m_navigationGroup && m_navigationGroup->button(pageIndex))
        m_navigationGroup->button(pageIndex)->setChecked(true);
}

void MainWindow::applyStyles()
{
    setStyleSheet(R"(
        QMainWindow, QWidget#centralSurface {
            background: #f4f6fa;
            color: #172033;
            font-family: "Segoe UI", "Arial";
            font-size: 10pt;
        }
        QWidget#sidebar {
            background: #17243a;
            color: #ffffff;
        }
        QLabel#brandMark {
            color: #ffffff;
            font-size: 23pt;
            font-weight: 800;
            letter-spacing: 1px;
        }
        QLabel#brandSubtitle, QLabel#sidebarSectionLabel, QLabel#versionLabel {
            color: #aab8cc;
            font-size: 8pt;
            font-weight: 700;
            letter-spacing: 1px;
        }
        QPushButton#navigationButton {
            text-align: left;
            color: #d6deea;
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 11px 12px;
            font-size: 10pt;
        }
        QPushButton#navigationButton:hover {
            background: #263750;
            color: #ffffff;
        }
        QPushButton#navigationButton:checked {
            color: #ffffff;
            background: #365fce;
            font-weight: 700;
        }
        QFrame#sidebarNote {
            background: #22334d;
            border: 1px solid #344963;
            border-radius: 10px;
        }
        QLabel#sidebarNoteTitle {
            color: #ffffff;
            font-weight: 700;
        }
        QLabel#sidebarNoteBody {
            color: #bdc9da;
            font-size: 9pt;
        }
        QLabel#pageTitle {
            color: #172033;
            font-size: 23pt;
            font-weight: 750;
        }
        QLabel#pageSubtitle, QLabel#panelHint, QLabel#metricDetail {
            color: #778398;
            font-size: 9pt;
        }
        QLabel#periodTag, QLabel#placeholderTag {
            color: #365fce;
            background: #e7edff;
            border-radius: 8px;
            padding: 7px 10px;
            font-size: 8pt;
            font-weight: 700;
        }
        QFrame#welcomePanel {
            background: #e9efff;
            border: 1px solid #d8e2ff;
            border-radius: 12px;
        }
        QLabel#welcomeTitle {
            color: #203a7c;
            font-size: 15pt;
            font-weight: 700;
        }
        QLabel#welcomeText {
            color: #52658f;
            font-size: 9pt;
        }
        QFrame#metricCard, QFrame#contentPanel {
            background: #ffffff;
            border: 1px solid #e4e9f1;
            border-radius: 12px;
        }
        QLabel#metricCaption {
            color: #778398;
            font-size: 8pt;
            font-weight: 700;
            letter-spacing: 0.5px;
        }
        QLabel#metricValue {
            color: #172033;
            font-size: 19pt;
            font-weight: 750;
        }
        QLabel#panelTitle {
            color: #172033;
            font-size: 13pt;
            font-weight: 700;
        }
        QListWidget#activityList {
            border: none;
            background: transparent;
            outline: none;
        }
        QListWidget#activityList::item {
            padding: 12px 5px;
            border-bottom: 1px solid #edf0f5;
            color: #39465b;
        }
        QLabel#budgetRowTitle {
            color: #344158;
            font-weight: 600;
        }
        QLabel#budgetRowValue {
            color: #64728a;
            font-size: 9pt;
        }
        QLabel#placeholderTitle {
            color: #172033;
            font-size: 18pt;
            font-weight: 700;
        }
        QLabel#placeholderDescription {
            color: #526076;
            font-size: 11pt;
        }
        QPushButton#primaryActionButton {
            color: #ffffff;
            background: #365fce;
            border: none;
            border-radius: 8px;
            padding: 10px 13px;
            font-weight: 700;
        }
        QPushButton#primaryActionButton:hover { background: #2b50b5; }
        QPushButton#secondaryActionButton {
            color: #263750;
            background: #ffffff;
            border: 1px solid #dce3ed;
            border-radius: 8px;
            padding: 10px 13px;
        }
        QPushButton#secondaryActionButton:hover { background: #edf2fa; }
        QPushButton#dangerActionButton {
            color: #a12a35;
            background: #fff1f1;
            border: 1px solid #f3d2d5;
            border-radius: 8px;
            padding: 10px 13px;
        }
        QPushButton#dangerActionButton:hover { background: #ffe4e6; }
        QTableWidget#accountsTable {
            background: #ffffff;
            alternate-background-color: #f7f9fc;
            border: 1px solid #e4e9f1;
            border-radius: 10px;
            gridline-color: #edf0f5;
            selection-background-color: #e7edff;
            selection-color: #172033;
        }
        QTableWidget#accountsTable QHeaderView::section {
            background: #f4f6fa;
            color: #526076;
            border: none;
            border-bottom: 1px solid #e4e9f1;
            padding: 10px 8px;
            font-weight: 700;
        }
        QLabel#fieldHint { color: #778398; font-size: 9pt; }
        QFrame#emptyStatePanel { background: #ffffff; border: 1px dashed #cbd5e1; border-radius: 12px; }
        QLabel#emptyStateTitle { color: #172033; font-size: 15pt; font-weight: 700; }
        QTableWidget#accountsTable::item { background: transparent; color: #263750; padding: 6px; border: none; }
        QTableWidget#accountsTable::item:selected { background: #dce7ff; color: #172033; }
        QTableWidget#accountsTable::item:focus { outline: none; }
        QTableWidget#accountsTable QTableCornerButton::section { background: #f4f6fa; border: none; }
        QPushButton:disabled { color: #9aa4b2; background: #f0f2f5; border-color: #e2e6ec; }
        QLabel#statusLabel {
            color: #7b8799;
            font-size: 8pt;
            padding-top: 5px;
        }
    )");
}
