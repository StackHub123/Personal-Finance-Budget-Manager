#include "mainwindow.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
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
    m_pages->addWidget(buildPlaceholderPage(
        "Accounts", "Create and manage the accounts used by your transactions."));
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
        QLabel#statusLabel {
            color: #7b8799;
            font-size: 8pt;
            padding-top: 5px;
        }
    )");
}
