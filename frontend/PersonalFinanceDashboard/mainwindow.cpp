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
    m_pages->addWidget(buildPlaceholderPage(
        "Transactions", "Record income and expenses, then review your transaction history."));
    m_pages->addWidget(buildAccountsPage());
    m_pages->addWidget(buildPlaceholderPage(
        "Budgets", "Set monthly category limits and compare them with actual spending."));
    m_pages->addWidget(buildPlaceholderPage(
        "Reports", "Review income, expenses, savings, and category-wise summaries."));
    m_pages->addWidget(buildPlaceholderPage(
        "Import statements", "Statement import is an optional enhancement and is not connected yet."));
    m_pages->addWidget(buildPlaceholderPage(
        "Backup & restore", "File serialization and restoration will be connected in a later phase."));
    m_pages->addWidget(buildPlaceholderPage(
        "Settings", "Application preferences and utilities will be added here."));

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
    layout->setSpacing(18);

    auto *welcome = new QFrame;
    welcome->setObjectName("welcomePanel");
    auto *welcomeLayout = new QVBoxLayout(welcome);
    welcomeLayout->setContentsMargins(22, 20, 22, 20);

    auto *welcomeTitle = makeLabel("Good financial habits start with visibility.",
                                   "welcomeTitle");
    auto *welcomeText = makeLabel(
        "This is the first frontend milestone. The values below are illustrative placeholders; "
        "they will be replaced by business-service data later.");
    welcomeText->setObjectName("welcomeText");
    welcomeText->setWordWrap(true);
    welcomeLayout->addWidget(welcomeTitle);
    welcomeLayout->addWidget(welcomeText);
    layout->addWidget(welcome);

    auto *metrics = new QHBoxLayout;
    metrics->setSpacing(14);
    metrics->addWidget(buildMetricCard("TOTAL BALANCE", "₹ 48,250", "Across demo accounts"));
    metrics->addWidget(buildMetricCard("INCOME", "₹ 32,000", "This month · demo"));
    metrics->addWidget(buildMetricCard("EXPENSES", "₹ 18,450", "This month · demo"));
    metrics->addWidget(buildMetricCard("NET SAVINGS", "₹ 13,550", "Income minus expenses"));
    layout->addLayout(metrics);

    auto *lower = new QHBoxLayout;
    lower->setSpacing(14);

    auto *activityPanel = new QFrame;
    activityPanel->setObjectName("contentPanel");
    auto *activityLayout = new QVBoxLayout(activityPanel);
    activityLayout->setContentsMargins(18, 18, 18, 18);
    auto *activityTitle = makeLabel("Recent activity", "panelTitle");
    auto *activityHint = makeLabel("Sample rows for layout preview", "panelHint");
    activityLayout->addWidget(activityTitle);
    activityLayout->addWidget(activityHint);

    auto *activity = new QListWidget;
    activity->setObjectName("activityList");
    activity->addItem("↓  Groceries                         − ₹ 1,250");
    activity->addItem("↑  Monthly allowance             + ₹ 8,000");
    activity->addItem("↓  Transport                          − ₹ 420");
    activity->addItem("↓  Books                                 − ₹ 890");
    activity->addItem("↓  Dining                               − ₹ 650");
    activityLayout->addWidget(activity, 1);

    auto *budgetPanel = new QFrame;
    budgetPanel->setObjectName("contentPanel");
    auto *budgetLayout = new QVBoxLayout(budgetPanel);
    budgetLayout->setContentsMargins(18, 18, 18, 18);
    budgetLayout->addWidget(makeLabel("Budget snapshot", "panelTitle"));
    budgetLayout->addWidget(makeLabel("Illustrative monthly limits", "panelHint"));

    const QList<QPair<QString, QString>> budgetRows = {
        {"Food & groceries", "₹ 4,200 of ₹ 6,000"},
        {"Transport", "₹ 1,300 of ₹ 2,000"},
        {"Entertainment", "₹ 1,800 of ₹ 1,500"}
    };

    for (const auto &row : budgetRows) {
        auto *rowTitle = makeLabel(row.first, "budgetRowTitle");
        auto *rowValue = makeLabel(row.second, "budgetRowValue");
        budgetLayout->addSpacing(8);
        budgetLayout->addWidget(rowTitle);
        budgetLayout->addWidget(rowValue);
    }
    auto *budgetNote = makeLabel("Budget calculations will be supplied by the business layer.",
                                 "panelHint");
    budgetNote->setWordWrap(true);
    budgetLayout->addStretch();
    budgetLayout->addWidget(budgetNote);

    lower->addWidget(activityPanel, 3);
    lower->addWidget(budgetPanel, 2);
    layout->addLayout(lower, 1);

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
    showAccountMessage(QString("Account '%1' deleted successfully.").arg(name), true);
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
