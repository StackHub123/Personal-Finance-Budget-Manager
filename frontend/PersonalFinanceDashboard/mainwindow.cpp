#include "mainwindow.h"
#include "models/Account.h"
#include <vector>
#include <stdexcept>
#include <limits>
#include <QCoreApplication>
#include <cmath>
#include <cstdint>
#include <exception>
#include <string>

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
#include <QPalette>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QSignalBlocker>
#include <QSettings>
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
#include <QPainter>
#include <QFontMetrics>
#include <QScrollArea>
#include <QLocale>

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

QString normalizedHeader(const QString &header)
{
    QString normalized;
    for (const QChar character : header) {
        if (character.isLetterOrNumber())
            normalized.append(character.toLower());
    }
    return normalized;
}

QString expandStatementTabs(const QString &line, int tabWidth = 8)
{
    QString expanded;
    int column = 0;
    for (const QChar character : line) {
        if (character == '\t') {
            const int spaces = tabWidth - (column % tabWidth);
            expanded += QString(spaces, ' ');
            column += spaces;
        } else {
            expanded += character;
            ++column;
        }
    }
    return expanded;
}

QString formatCurrency(double amount, bool indianGrouping, int decimals = 2)
{
    const QLocale locale = indianGrouping
        ? QLocale(QLocale::English, QLocale::India)
        : QLocale(QLocale::English, QLocale::UnitedStates);
    return QString("₹ %1").arg(locale.toString(amount, 'f', decimals));
}

QDate parseStatementDate(const QString &value)
{
    const QString text = value.trimmed();
    const QDate isoDate = QDate::fromString(text, Qt::ISODate);
    if (isoDate.isValid())
        return isoDate;
    const QStringList formats = {
        "d/M/yy", "dd/MM/yy", "d/M/yyyy", "dd/MM/yyyy",
        "d-M-yy", "dd-MM-yy", "d-M-yyyy", "dd-MM-yyyy",
        "yyyy/M/d", "yyyy/MM/dd", "d.M.yy", "dd.MM.yyyy",
        "d-MMM-yyyy", "dd-MMM-yyyy", "d MMM yyyy", "dd MMM yyyy"
    };
    for (const QString &format : formats) {
        const QDate date = QDate::fromString(text, format);
        if (date.isValid())
            return date;
    }
    return {};
}

bool parseStatementAmount(const QString &rawValue, double &amount)
{
    QString text = rawValue.trimmed();
    if (text.isEmpty() || text == "-")
        return false;

    const bool parenthesizedNegative = text.startsWith('(') && text.endsWith(')');
    if (parenthesizedNegative)
        text = text.mid(1, text.size() - 2);
    text.remove(QRegularExpression("[^0-9,.-]"));
    if (text.isEmpty())
        return false;

    const int lastComma = text.lastIndexOf(',');
    const int lastPeriod = text.lastIndexOf('.');
    if (lastComma >= 0 && lastPeriod >= 0) {
        if (lastComma > lastPeriod) {
            text.remove('.');
            text.replace(',', '.');
        } else {
            text.remove(',');
        }
    } else if (lastComma >= 0) {
        const int digitsAfterComma = text.size() - lastComma - 1;
        if (text.count(',') == 1 && digitsAfterComma > 0 && digitsAfterComma <= 2)
            text.replace(',', '.');
        else
            text.remove(',');
    }

    bool ok = false;
    double parsed = text.toDouble(&ok);
    if (!ok || !qIsFinite(parsed))
        return false;
    amount = parenthesizedNegative ? -qAbs(parsed) : parsed;
    return true;
}

bool statementTypeIsIncome(const QString &type, double signedAmount)
{
    const QString normalized = type.trimmed().toLower();
    if (normalized == "income" || normalized == "credit" || normalized == "cr" ||
        normalized == "deposit" || normalized == "salary" || normalized == "paid in")
        return true;
    if (normalized == "expense" || normalized == "debit" || normalized == "dr" ||
        normalized == "withdrawal" || normalized == "payment" ||
        normalized == "purchase" || normalized == "paid out")
        return false;
    return signedAmount > 0.0;
}
}

class DonutChartWidget final : public QWidget
{
public:
    explicit DonutChartWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(220);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void setValues(const QVector<QPair<QString, double>> &values)
    {
        m_values = values;
        update();
    }

    void setDarkMode(bool dark)
    {
        m_darkMode = dark;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF bounds = QRectF(rect()).adjusted(4, 8, -4, -8);
        double total = 0.0;
        for (const auto &entry : m_values)
            total += qMax(0.0, entry.second);

        if (total <= 0.0) {
            painter.setPen(m_darkMode ? QColor("#a4b2c5") : QColor("#8994a7"));
            painter.drawText(bounds, Qt::AlignCenter,
                             "No expense data for this period");
            return;
        }

        const qreal diameter = qMin(bounds.height() - 12.0,
                                    qMax(110.0, bounds.width() * 0.42));
        const QRectF pie(bounds.left(), bounds.center().y() - diameter / 2.0,
                         diameter, diameter);
        const QVector<QColor> colors = {
            QColor("#3b82f6"), QColor("#14b8a6"), QColor("#f59e0b"),
            QColor("#8b5cf6"), QColor("#f97316"), QColor("#ec4899"),
            QColor("#64748b")
        };
        int startAngle = 90 * 16;
        for (int i = 0; i < m_values.size(); ++i) {
            const double value = qMax(0.0, m_values.at(i).second);
            if (value <= 0.0)
                continue;
            const int span = qRound(value / total * 360.0 * 16.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.at(i % colors.size()));
            painter.drawPie(pie, startAngle, -span);
            startAngle -= span;
        }
        painter.setBrush(m_darkMode ? QColor("#121e30") : QColor(Qt::white));
        painter.drawEllipse(pie.adjusted(diameter * 0.27, diameter * 0.27,
                                         -diameter * 0.27, -diameter * 0.27));

        const qreal legendX = pie.right() + 18.0;
        const qreal legendWidth = qMax(0.0, bounds.right() - legendX);
        const qreal rowHeight = qMin(30.0, bounds.height() /
                                     qMax(1, static_cast<int>(m_values.size())));
        qreal y = bounds.center().y() - rowHeight * m_values.size() / 2.0;
        const QFontMetrics metrics(painter.font());
        for (int i = 0; i < m_values.size(); ++i) {
            const auto &entry = m_values.at(i);
            if (entry.second <= 0.0)
                continue;
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.at(i % colors.size()));
            painter.drawRoundedRect(QRectF(legendX, y + 7, 10, 10), 3, 3);
            painter.setPen(m_darkMode ? QColor("#dce6f4") : QColor("#344054"));
            QString label = metrics.elidedText(entry.first, Qt::ElideRight,
                                                qMax(0, static_cast<int>(legendWidth - 72)));
            painter.drawText(QRectF(legendX + 17, y, qMax(0.0, legendWidth - 68), rowHeight),
                             Qt::AlignVCenter | Qt::AlignLeft, label);
            painter.setPen(m_darkMode ? QColor("#a4b2c5") : QColor("#667085"));
            painter.drawText(QRectF(bounds.right() - 48, y, 48, rowHeight),
                             Qt::AlignVCenter | Qt::AlignRight,
                             QString::number(entry.second / total * 100.0, 'f', 0) + "%");
            y += rowHeight;
        }
    }

private:
    QVector<QPair<QString, double>> m_values;
    bool m_darkMode = false;
};

class HorizontalBarChartWidget final : public QWidget
{
public:
    explicit HorizontalBarChartWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(220);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void setValues(const QVector<QPair<QString, double>> &values)
    {
        m_values = values;
        setMinimumHeight(qMax(220, 52 * qMin(5, static_cast<int>(values.size())) + 24));
        updateGeometry();
        update();
    }

    void setIndianNumberFormat(bool indian)
    {
        m_indianNumberFormat = indian;
        update();
    }

    void setDarkMode(bool dark)
    {
        m_darkMode = dark;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF bounds = QRectF(rect()).adjusted(6, 8, -6, -8);
        if (m_values.isEmpty()) {
            painter.setPen(m_darkMode ? QColor("#a4b2c5") : QColor("#8994a7"));
            painter.drawText(bounds, Qt::AlignCenter, "No report data for this period");
            return;
        }

        double maxValue = 0.0;
        for (const auto &entry : m_values)
            maxValue = qMax(maxValue, entry.second);
        if (maxValue <= 0.0)
            maxValue = 1.0;

        const int count = qMin(5, static_cast<int>(m_values.size()));
        const qreal rowHeight = bounds.height() / count;
        const qreal labelWidth = qMin(118.0, bounds.width() * 0.34);
        const qreal valueWidth = 82.0;
        const qreal barLeft = bounds.left() + labelWidth + 8;
        const qreal barWidth = qMax(24.0, bounds.width() - labelWidth - valueWidth - 20);
        const QFontMetrics metrics(painter.font());
        for (int i = 0; i < count; ++i) {
            const auto &entry = m_values.at(i);
            const qreal y = bounds.top() + i * rowHeight;
            painter.setPen(m_darkMode ? QColor("#c4d0e0") : QColor("#475467"));
            painter.drawText(QRectF(bounds.left(), y, labelWidth, rowHeight),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             metrics.elidedText(entry.first, Qt::ElideRight,
                                                static_cast<int>(labelWidth)));
            const QRectF track(barLeft, y + rowHeight * 0.34, barWidth, 12);
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_darkMode ? QColor("#26364b") : QColor("#edf2f7"));
            painter.drawRoundedRect(track, 6, 6);
            QRectF amount = track;
            amount.setWidth(track.width() * qBound(0.0, entry.second / maxValue, 1.0));
            painter.setBrush(m_darkMode ? QColor("#4c88f0") : QColor("#3b82f6"));
            painter.drawRoundedRect(amount, 6, 6);
            painter.setPen(m_darkMode ? QColor("#dce6f4") : QColor("#344054"));
            painter.drawText(QRectF(bounds.right() - valueWidth, y, valueWidth, rowHeight),
                             Qt::AlignVCenter | Qt::AlignRight,
                             formatCurrency(entry.second, m_indianNumberFormat, 0));
        }
    }

private:
    QVector<QPair<QString, double>> m_values;
    bool m_indianNumberFormat = true;
    bool m_darkMode = false;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_accountRepository(m_database),
      m_transactionRepository(m_database),
      m_budgetRepository(m_database),
      m_financeService(m_accountRepository, m_transactionRepository),
      m_transactionService(m_accountRepository, m_transactionRepository),
      m_budgetService(m_budgetRepository, m_transactionRepository),
      m_backupRestoreService(m_database, m_accountRepository,
                             m_transactionRepository, m_budgetRepository)
{
    QCoreApplication::setOrganizationName("PFBM");
    QCoreApplication::setApplicationName("PersonalFinanceDashboard");
    QSettings settings;
    m_activeAccountId = settings.value("dashboard/activeAccountId", 0).toInt();
    m_themePreference = settings.value("settings/theme", "light").toString();
    m_dateDisplayFormat = settings.value("settings/dateFormat", "yyyy-MM-dd").toString();
    m_numberFormat = settings.value("settings/numberFormat", "indian").toString();
    m_defaultReportPeriod = settings.value("settings/defaultReportPeriod", "This month").toString();
    m_importDuplicatePolicy = settings.value("settings/importDuplicatePolicy", "flag").toString();
    if (m_themePreference != "light" && m_themePreference != "dark")
        m_themePreference = "light";
    if (m_dateDisplayFormat != "dd/MM/yyyy" && m_dateDisplayFormat != "yyyy-MM-dd" &&
        m_dateDisplayFormat != "d MMM yyyy")
        m_dateDisplayFormat = "yyyy-MM-dd";
    if (m_numberFormat != "indian" && m_numberFormat != "western")
        m_numberFormat = "indian";
    if (m_defaultReportPeriod != "This month" && m_defaultReportPeriod != "Last month" &&
        m_defaultReportPeriod != "All time")
        m_defaultReportPeriod = "This month";
    if (m_importDuplicatePolicy != "flag" && m_importDuplicatePolicy != "skip" &&
        m_importDuplicatePolicy != "import")
        m_importDuplicatePolicy = "flag";

    const QString dataDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (dataDirectory.isEmpty() || !QDir().mkpath(dataDirectory)) {
        m_databaseError = "Could not create the application data directory.";
    } else {
        const QString databasePath =
            QDir(dataDirectory).filePath("personal_finance.db");

        std::string error;

        if (!m_database.open(databasePath.toStdString(), error)) {
            m_databaseError = QString::fromStdString(error);
        } else if (!m_database.initializeSchema(error)) {
            m_databaseError = QString::fromStdString(error);
        } else {
            m_databaseReady = true;
        }
    }

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

    auto *activeAccountLabel = makeLabel("ACTIVE ACCOUNT", "periodTag");
    header->addWidget(activeAccountLabel, 0, Qt::AlignVCenter);
    m_activeAccountCombo = new QComboBox;
    m_activeAccountCombo->setObjectName("activeAccountCombo");
    m_activeAccountCombo->setMinimumWidth(190);
    m_activeAccountCombo->setMaximumWidth(270);
    m_activeAccountCombo->setToolTip(
        "Choose the account whose transactions, budgets, dashboard and reports you are viewing.");
    header->addWidget(m_activeAccountCombo, 0, Qt::AlignVCenter);
    connect(m_activeAccountCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::activeAccountChanged);
    mainLayout->addLayout(header);

    m_pages = new QStackedWidget;
    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildTransactionsPage());
    m_pages->addWidget(buildAccountsPage());
    m_pages->addWidget(buildBudgetsPage());
    m_pages->addWidget(buildReportsPage());
    m_pages->addWidget(buildImportStatementsPage());
    m_pages->addWidget(buildUtilitiesPage());
    m_pages->addWidget(buildSettingsPage());

    mainLayout->addWidget(m_pages, 1);

    const QString statusText = m_databaseReady
        ? QString("SQLite connected • Account data is saved persistently")
        : QString("Database connection failed: %1").arg(m_databaseError);

    m_statusLabel = makeLabel(statusText, "statusLabel");
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
    QString activeAccountName = "No account selected";
    for (const auto &account : m_accounts) {
        if (account.id == m_activeAccountId) {
            activeAccountName = account.name;
            break;
        }
    }
    const QString welcomeMessage = m_activeAccountId > 0
        ? QString("Showing finances for %1. Dashboard figures, recent activity and budget usage are limited to this account. Export a backup from Utilities to keep a copy.").arg(activeAccountName)
        : "Create an account to begin tracking your finances.";
    auto *welcomeText = makeLabel(welcomeMessage, "welcomeText");
    welcomeText->setWordWrap(true); wl->addWidget(welcomeText); layout->addWidget(welcome);

    double accountBalance=0, income=0, expenses=0;
    for (const auto &a : m_accounts) {
        if (a.id == m_activeAccountId)
            accountBalance += a.balance;
    }
    for (const auto &t : m_transactions) {
        if (t.accountId != m_activeAccountId) continue;
        if(t.type=="Income") income+=t.amount; else expenses+=t.amount;
    }
    const double currentBalance = accountBalance + income - expenses;
    auto *metrics = new QHBoxLayout; metrics->setSpacing(12);
    metrics->addWidget(buildMetricCard("CURRENT BALANCE", formatMoney(currentBalance), "Opening balances + transactions"));
    metrics->addWidget(buildMetricCard("TOTAL INCOME", formatMoney(income), "Recorded income"));
    metrics->addWidget(buildMetricCard("TOTAL EXPENSES", formatMoney(expenses), "Recorded expenses"));
    metrics->addWidget(buildMetricCard("NET SAVINGS", formatMoney(income-expenses), "Income minus expenses"));
    layout->addLayout(metrics);

    auto *lower = new QHBoxLayout; lower->setSpacing(14);
    auto *activityPanel = new QFrame; activityPanel->setObjectName("contentPanel");
    auto *al = new QVBoxLayout(activityPanel); al->setContentsMargins(18,18,18,18);
    al->addWidget(makeLabel("Recent activity", "panelTitle"));
    auto *activity = new QTableWidget(0,4); activity->setObjectName("accountsTable");
    activity->setHorizontalHeaderLabels({"Date","Description","Category","Amount"});
    activity->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    activity->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QVector<TransactionRecord> recent;
    for (const auto &transaction : m_transactions) {
        if (transaction.accountId == m_activeAccountId)
            recent.append(transaction);
    }
    std::sort(recent.begin(),recent.end(),[](const TransactionRecord &a,const TransactionRecord &b){return a.date>b.date;});
    for(int i=0;i<qMin(5,recent.size());++i){const auto &t=recent[i];int r=activity->rowCount();activity->insertRow(r);activity->setItem(r,0,new QTableWidgetItem(formatDate(t.date)));activity->setItem(r,1,new QTableWidgetItem(t.description));activity->setItem(r,2,new QTableWidgetItem(t.category));activity->setItem(r,3,new QTableWidgetItem(QString("%1 %2").arg(t.type=="Income"?"+":"−",formatMoney(t.amount))));}
    if(recent.isEmpty()) al->addWidget(makeLabel("No transactions yet. Add income or an expense to see your activity.","panelHint"));
    activity->setVisible(!recent.isEmpty());
    al->addWidget(activity,1);

    auto *budgetPanel = new QFrame; budgetPanel->setObjectName("contentPanel");
    auto *bl = new QVBoxLayout(budgetPanel); bl->setContentsMargins(18,18,18,18);
    bl->addWidget(makeLabel("Budget usage", "panelTitle"));
    bool overBudget=false;
    int visibleBudgetCount = 0;
    for (const auto &b : m_budgets) {
        if (m_activeAccountId <= 0 || b.accountId != m_activeAccountId)
            continue;
        double spent = 0;
        for (const auto &t : m_transactions) {
            if (t.accountId == b.accountId &&
                t.type == "Expense" &&
                t.category.compare(b.category, Qt::CaseInsensitive) == 0 &&
                t.date.left(7) == b.month) {
                spent += t.amount;
            }
        }

        ++visibleBudgetCount;
        auto *label = makeLabel(QString("%1 · %2").arg(b.category, b.month),
                                "budgetRowTitle");
        label->setWordWrap(true);
        bl->addWidget(label);

        const double remaining = b.limit - spent;
        const QString amountText = remaining >= 0.0
            ? QString("%1 spent · %2 left · %3 limit")
                  .arg(formatMoney(spent), formatMoney(remaining), formatMoney(b.limit))
            : QString("%1 spent · Over by %2 · %3 limit")
                  .arg(formatMoney(spent), formatMoney(-remaining), formatMoney(b.limit));
        auto *amountLabel = makeLabel(amountText, "budgetRowValue");
        amountLabel->setWordWrap(true);
        bl->addWidget(amountLabel);

        auto *bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setValue(b.limit > 0
            ? qBound(0, static_cast<int>(spent / b.limit * 100), 100)
            : 0);
        bar->setFormat("%p% used");
        bl->addWidget(bar);

        if (spent > b.limit) {
            overBudget = true;
            auto *warning = makeLabel(
                QString("Over budget: %1 · %2").arg(b.accountName, b.category),
                "panelHint");
            warning->setStyleSheet("color:#b42318;font-weight:700;");
            bl->addWidget(warning);
        }
    }
    if (visibleBudgetCount == 0)
        bl->addWidget(makeLabel("No budgets for this account yet. Create a budget to track category spending.", "panelHint"));
    else if (!overBudget)
        bl->addWidget(makeLabel("All budgets are within their limits.", "panelHint"));
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
        "Account data is saved locally in the SQLite database.", "panelHint");
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

    m_accounts.clear();
    m_nextAccountId = 1;

    if (!m_databaseReady) {
        if (m_statusLabel) {
            m_statusLabel->setText(
                QString("Database unavailable: %1").arg(m_databaseError));
        }
    } else {
        std::string error;
        const auto accounts = m_accountRepository.getAll(error);

        if (!error.empty()) {
            if (m_statusLabel) {
                m_statusLabel->setText(
                    QString("Could not load accounts: %1")
                        .arg(QString::fromStdString(error)));
            }
        } else {
            for (const Account &account : accounts) {
                AccountRecord record;
                record.id = account.getId();
                record.name = QString::fromStdString(account.getName());
                record.type = QString::fromStdString(account.getType());
                record.balance =
                    static_cast<double>(account.getInitialBalancePaise())
                    / 100.0;

                m_accounts.append(record);

                if (record.id >= m_nextAccountId)
                    m_nextAccountId = record.id + 1;
            }
        }
    }

    m_accountsTable->setRowCount(0);

    for (const AccountRecord &account : m_accounts) {
        const int row = m_accountsTable->rowCount();
        m_accountsTable->insertRow(row);

        auto *idItem = new QTableWidgetItem(QString::number(account.id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_accountsTable->setItem(row, 0, idItem);
        m_accountsTable->setItem(
            row, 1, new QTableWidgetItem(account.name));
        m_accountsTable->setItem(
            row, 2, new QTableWidgetItem(account.type));

        auto *balanceItem = new QTableWidgetItem(
            formatMoney(account.balance));
        balanceItem->setTextAlignment(
            Qt::AlignRight | Qt::AlignVCenter);
        m_accountsTable->setItem(row, 3, balanceItem);
    }

    const bool hasAccounts = !m_accounts.isEmpty();
    m_accountsTable->setVisible(hasAccounts);

    if (m_emptyState)
        m_emptyState->setVisible(!hasAccounts);

    if (m_editAccountButton)
        m_editAccountButton->setEnabled(false);

    if (m_deleteAccountButton)
        m_deleteAccountButton->setEnabled(false);

    refreshActiveAccountCombo();
    refreshStatementAccountCombo();
}

void MainWindow::refreshActiveAccountCombo()
{
    if (!m_activeAccountCombo)
        return;

    const QSignalBlocker blocker(m_activeAccountCombo);
    m_activeAccountCombo->clear();

    if (m_accounts.isEmpty()) {
        m_activeAccountCombo->addItem("No accounts available", 0);
        m_activeAccountCombo->setEnabled(false);
        m_activeAccountId = 0;
        QSettings settings;
        settings.setValue("dashboard/activeAccountId", 0);
        return;
    }

    m_activeAccountCombo->setEnabled(true);
    for (const AccountRecord &account : m_accounts)
        m_activeAccountCombo->addItem(account.name, account.id);

    int selectedIndex = m_activeAccountCombo->findData(m_activeAccountId);
    if (selectedIndex < 0)
        selectedIndex = 0;

    m_activeAccountCombo->setCurrentIndex(selectedIndex);
    m_activeAccountId = m_activeAccountCombo->currentData().toInt();

    QSettings settings;
    settings.setValue("dashboard/activeAccountId", m_activeAccountId);
}

void MainWindow::activeAccountChanged(int index)
{
    Q_UNUSED(index);
    if (!m_activeAccountCombo)
        return;

    const int selectedAccountId = m_activeAccountCombo->currentData().toInt();
    if (selectedAccountId <= 0 || selectedAccountId == m_activeAccountId)
        return;

    if (!m_statementPath.isEmpty())
        clearStatementPreview(
            "The active account changed. Choose the statement again to preview it for the new account.");

    m_activeAccountId = selectedAccountId;
    QSettings settings;
    settings.setValue("dashboard/activeAccountId", m_activeAccountId);

    refreshAccountScopedViews();

    QString accountName = m_activeAccountCombo->currentText();
    if (m_statusLabel)
        m_statusLabel->setText(
            QString("SQLite connected • Active account: %1").arg(accountName));
}

void MainWindow::refreshAccountScopedViews()
{
    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();
    if (m_reportWasGenerated)
        runReport();
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
    styleComboBoxPopup(typeCombo);
    auto *typeHint = makeLabel(
        "Choose the option that best describes this account. For everyday cash, choose Cash / Wallet.",
        "fieldHint");
    typeHint->setWordWrap(true);
    auto *balanceSpin = new QDoubleSpinBox(&dialog);
    balanceSpin->setLocale(m_numberFormat == "indian"
        ? QLocale(QLocale::English, QLocale::India)
        : QLocale(QLocale::English, QLocale::UnitedStates));
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

    if (!m_databaseReady) {
        showAccountMessage(
            QString("Database unavailable: %1").arg(m_databaseError),
            false);
        return;
    }

    const QString name = nameEdit->text().trimmed();
    const QString type = typeCombo->currentText();

    const auto balancePaise = static_cast<std::int64_t>(
        std::llround(balanceSpin->value() * 100.0));

    try {
        Account account(
            0,
            name.toStdString(),
            type.toStdString(),
            balancePaise);

        std::string error;

        if (!m_accountRepository.add(account, error)) {
            showAccountMessage(
                QString("Could not save account: %1")
                    .arg(QString::fromStdString(error)),
                false);
            return;
        }

        refreshAccountsTable();
        refreshAccountScopedViews();

        showAccountMessage(
            QString("'%1' was saved successfully.").arg(name), true);
    } catch (const std::exception &exception) {
        showAccountMessage(
            QString("Invalid account: %1").arg(exception.what()), false);
    }
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
    styleComboBoxPopup(typeCombo);
    typeCombo->setCurrentText(original.type == "Savings" ? "Savings account" : original.type == "Cash" ? "Cash / Wallet" : original.type == "Current" ? "Current account" : original.type);
    auto *balanceSpin = new QDoubleSpinBox(&dialog);
    balanceSpin->setLocale(m_numberFormat == "indian"
        ? QLocale(QLocale::English, QLocale::India)
        : QLocale(QLocale::English, QLocale::UnitedStates));
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
    if (!m_databaseReady) {
        showAccountMessage(
            QString("Database unavailable: %1").arg(m_databaseError),
            false);
        return;
    }

    const QString name = nameEdit->text().trimmed();
    const QString type = typeCombo->currentText();

    const auto balancePaise = static_cast<std::int64_t>(
        std::llround(balanceSpin->value() * 100.0));

    try {
        Account updatedAccount(
            id,
            name.toStdString(),
            type.toStdString(),
            balancePaise);

        std::string error;

        if (!m_accountRepository.update(updatedAccount, error)) {
            showAccountMessage(
                QString("Could not update account: %1")
                    .arg(QString::fromStdString(error)),
                false);
            return;
        }

        refreshAccountsTable();
        refreshAccountScopedViews();

        showAccountMessage(
            QString("'%1' was updated successfully.").arg(name), true);
    } catch (const std::exception &exception) {
        showAccountMessage(
            QString("Invalid account: %1").arg(exception.what()), false);
    }
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

    if (!m_databaseReady) {
        showAccountMessage(
            QString("Database unavailable: %1").arg(m_databaseError),
            false);
        return;
    }

    std::string error;

    if (!m_accountRepository.remove(id, error)) {
        showAccountMessage(
            QString("Could not delete account: %1")
                .arg(QString::fromStdString(error)),
            false);
        return;
    }

    refreshAccountsTable();
    refreshAccountScopedViews();

    showAccountMessage(
        QString("Account '%1' deleted successfully.").arg(name), true);
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
    accountForm->addRow("Active import account", m_statementAccountCombo);
    layout->addLayout(accountForm);
    m_statementAccountNotice = makeLabel(
        "Imported transactions are saved to the active account. Change the active account at the top before choosing a statement.", "panelHint");
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
        "CSV imports support common date, description, amount, debit and credit headings. "
        "The importer detects comma, semicolon, tab and pipe delimiters, and skips statement title rows. "
        "PDF and Excel files must be exported as CSV first.",
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
        "Valid imported rows are saved to SQLite and linked to the selected account. "
        "Repeated transactions are preserved as separate entries. "
        "Rows with invalid dates or amounts are reported and skipped.",
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

    const QSignalBlocker blocker(m_statementAccountCombo);
    m_statementAccountCombo->clear();
    m_statementAccountCombo->addItem("Select an account...", -1);
    for (const AccountRecord &account : m_accounts)
        m_statementAccountCombo->addItem(account.name, account.id);

    int selectedIndex = m_statementAccountCombo->findData(m_activeAccountId);
    if (selectedIndex < 0)
        selectedIndex = 0;
    m_statementAccountCombo->setCurrentIndex(selectedIndex);

    const int selectedAccountId = m_statementAccountCombo->currentData().toInt();
    if (!m_statementPath.isEmpty() && m_statementAccountId > 0 &&
        selectedAccountId != m_statementAccountId) {
        clearStatementPreview(
            "The active account changed. Choose the statement again to preview it for that account.");
    }

    const bool hasAccounts = !m_accounts.isEmpty();
    const bool hasSelectedAccount = m_statementAccountCombo->currentIndex() > 0;
    m_statementAccountCombo->setEnabled(false);
    m_statementAccountCombo->setToolTip(
        "The import target is controlled by the Active Account selector at the top of the application.");
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
            ? QString("Every imported transaction will be linked to the active account: %1. Change the Active Account selector at the top if needed.").arg(m_statementAccountCombo->currentText())
            : "No accounts exist yet. Open Accounts and create a bank account or cash wallet first. Statement import is disabled until an account exists.");
    }

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

QStringList MainWindow::splitCsvLine(const QString &line, QChar delimiter) const
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
        } else if (ch == delimiter && !inQuotes) {
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
            "No account exists yet. Go to Accounts and create the account these transactions belong to.");
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
    refreshTransactionsTable();
    QTextStream stream(&file);
    const QStringList lines = stream.readAll().split(QRegularExpression("\\r?\\n"));
    bool headerFound = false;
    int dateColumn = -1, descriptionColumn = -1, amountColumn = -1;
    int debitColumn = -1, creditColumn = -1, typeColumn = -1;
    QChar delimiter = ',';
    bool fixedWidthMode = false;
    QVector<int> fixedColumnStarts;
    const QVector<QChar> delimiters = {',', ';', '\t', '|'};
    const QSet<QString> dateHeaders = {
        "date", "transactiondate", "txndate", "valuedate", "valuedt",
        "bookingdate", "postdate", "postingdate", "dateposted"
    };
    const QSet<QString> descriptionHeaders = {
        "description", "details", "narration", "merchant", "particulars",
        "transactiondetails", "transactiondescription", "remarks", "payee",
        "narrative", "memo", "merchantname"
    };
    const QSet<QString> amountHeaders = {
        "amount", "transactionamount", "amt", "amountinr", "amountusd"
    };
    const QSet<QString> debitHeaders = {
        "debit", "debitamount", "withdrawal", "withdrawalamount", "withdrawalamt",
        "paidout", "dramount", "dramountinr", "moneyout", "withdrawalinr"
    };
    const QSet<QString> creditHeaders = {
        "credit", "creditamount", "deposit", "depositamount", "depositamt",
        "paidin", "cramount", "cramountinr", "moneyin", "depositinr"
    };
    const QSet<QString> typeHeaders = {
        "type", "transactiontype", "drcr", "crdr", "debitcredit"
    };

    int headerLineIndex = -1;
    for (int lineIndex = 0; lineIndex < lines.size() && !headerFound; ++lineIndex) {
        if (lines.at(lineIndex).trimmed().isEmpty())
            continue;
        const QString expandedHeader = expandStatementTabs(
            lines.at(lineIndex).toUpper());
        const QStringList fixedHeaderNames = {
            "DATE", "NARRATION", "CHQ./REF.NO.", "VALUE DT",
            "WITHDRAWAL AMT.", "DEPOSIT AMT.", "CLOSING BALANCE"
        };
        QVector<int> fixedStarts;
        for (const QString &name : fixedHeaderNames)
            fixedStarts.append(expandedHeader.indexOf(name));
        if (lines.at(lineIndex).contains('\t') &&
            fixedStarts.size() == fixedHeaderNames.size() &&
            fixedStarts.at(0) >= 0 && fixedStarts.at(1) > fixedStarts.at(0) &&
            fixedStarts.at(4) > fixedStarts.at(1) &&
            fixedStarts.at(5) > fixedStarts.at(4) &&
            fixedStarts.at(6) > fixedStarts.at(5)) {
            fixedWidthMode = true;
            fixedColumnStarts = fixedStarts;
            dateColumn = 0;
            descriptionColumn = 1;
            debitColumn = 4;
            creditColumn = 5;
            headerLineIndex = lineIndex;
            headerFound = true;
            break;
        }

        for (const QChar candidateDelimiter : delimiters) {
            const QStringList fields = splitCsvLine(lines.at(lineIndex), candidateDelimiter);
            int candidateDate = -1, candidateDescription = -1, candidateAmount = -1;
            int candidateDebit = -1, candidateCredit = -1, candidateType = -1;
            for (int column = 0; column < fields.size(); ++column) {
                QString heading = fields.at(column).trimmed();
                if (column == 0 && !heading.isEmpty() && heading.at(0) == QChar(0xFEFF))
                    heading.remove(0, 1);
                const QString key = normalizedHeader(heading);
                if (dateHeaders.contains(key) || key.startsWith("transactiondate")) candidateDate = column;
                else if (descriptionHeaders.contains(key) || key.startsWith("transactiondescription")) candidateDescription = column;
                else if (amountHeaders.contains(key) || key.startsWith("amount")) candidateAmount = column;
                else if (typeHeaders.contains(key) || key.startsWith("transactiontype")) candidateType = column;
                else if (debitHeaders.contains(key) || key.startsWith("debit") || key.startsWith("withdrawal")) candidateDebit = column;
                else if (creditHeaders.contains(key) || key.startsWith("credit") || key.startsWith("deposit")) candidateCredit = column;
            }
            const bool hasAmount = candidateAmount >= 0 ||
                                   candidateDebit >= 0 || candidateCredit >= 0;
            if (candidateDate >= 0 && candidateDescription >= 0 && hasAmount) {
                dateColumn = candidateDate;
                descriptionColumn = candidateDescription;
                amountColumn = candidateAmount;
                debitColumn = candidateDebit;
                creditColumn = candidateCredit;
                typeColumn = candidateType;
                delimiter = candidateDelimiter;
                headerLineIndex = lineIndex;
                headerFound = true;
                break;
            }
        }
    }

    auto fieldAt = [](const QStringList &fields, int column) {
        return column >= 0 && column < fields.size()
            ? fields.at(column).trimmed() : QString();
    };
    auto isRecognizedHeader = [&](const QStringList &fields) {
        bool hasDate = false, hasDescription = false, hasAmount = false;
        for (const QString &field : fields) {
            const QString key = normalizedHeader(field);
            hasDate = hasDate || dateHeaders.contains(key) || key.startsWith("transactiondate");
            hasDescription = hasDescription || descriptionHeaders.contains(key) ||
                             key.startsWith("transactiondescription");
            hasAmount = hasAmount || amountHeaders.contains(key) || key.startsWith("amount") ||
                        debitHeaders.contains(key) || key.startsWith("debit") ||
                        key.startsWith("withdrawal") || creditHeaders.contains(key) ||
                        key.startsWith("credit") || key.startsWith("deposit");
        }
        return hasDate && hasDescription && hasAmount;
    };
    auto rowMeansIncome = [](const StatementRecord &record) {
        return statementTypeIsIncome(record.type, record.amount);
    };

    if (headerFound) {
        for (int lineIndex = headerLineIndex + 1; lineIndex < lines.size(); ++lineIndex) {
            const int lineNumber = lineIndex + 1;
            if (lines.at(lineIndex).trimmed().isEmpty())
                continue;
            QStringList fields;
            if (fixedWidthMode) {
                const QString expandedLine = expandStatementTabs(lines.at(lineIndex));
                for (int column = 0; column < fixedColumnStarts.size(); ++column) {
                    const int start = fixedColumnStarts.at(column);
                    const int end = column + 1 < fixedColumnStarts.size()
                        ? fixedColumnStarts.at(column + 1) : expandedLine.size();
                    fields.append(start < expandedLine.size()
                        ? expandedLine.mid(start, qMax(0, end - start)).trimmed()
                        : QString());
                }
            } else {
                fields = splitCsvLine(lines.at(lineIndex), delimiter);
            }
            if (isRecognizedHeader(fields))
                continue; // Bank exports sometimes repeat column labels on each page.

            StatementRecord record;
            const QString dateText = fieldAt(fields, dateColumn);
            record.description = fieldAt(fields, descriptionColumn);
            record.lineNumber = lineNumber;
            QString dateWithoutSeparators = dateText;
            dateWithoutSeparators.remove(QRegularExpression("[\\s-]"));
            const QString summaryText = (dateText + " " + record.description).toLower();
            if ((!dateText.trimmed().isEmpty() && dateWithoutSeparators.isEmpty()) ||
                summaryText.contains("opening balance") ||
                summaryText.contains("closing balance") ||
                summaryText.contains("grand total"))
                continue;

            bool amountOk = false;
            if (debitColumn >= 0 || creditColumn >= 0) {
                const QString debitText = fieldAt(fields, debitColumn);
                const QString creditText = fieldAt(fields, creditColumn);
                double debit = 0.0, credit = 0.0;
                const bool hasDebit = !debitText.isEmpty() && debitText != "-";
                const bool hasCredit = !creditText.isEmpty() && creditText != "-";
                if (hasDebit && hasCredit) {
                    ++m_statementInvalidRowCount;
                    errors << QString("Line %1: both debit and credit have values; row skipped.")
                                  .arg(lineNumber);
                    continue;
                }
                if (!hasDebit && !hasCredit)
                    continue; // Ignore statement totals and non-transaction rows.
                if (hasDebit) {
                    amountOk = parseStatementAmount(debitText, debit);
                    record.amount = -qAbs(debit);
                    record.type = "Debit";
                } else {
                    amountOk = parseStatementAmount(creditText, credit);
                    record.amount = qAbs(credit);
                    record.type = "Credit";
                }
            } else {
                const QString amountText = fieldAt(fields, amountColumn);
                if (amountText.isEmpty() || amountText == "-")
                    continue;
                amountOk = parseStatementAmount(amountText, record.amount);
                record.type = fieldAt(fields, typeColumn);
            }

            const QDate parsedDate = parseStatementDate(dateText);
            if (!parsedDate.isValid()) {
                ++m_statementInvalidRowCount;
                errors << QString("Line %1: unrecognized transaction date; row skipped.")
                              .arg(lineNumber);
                continue;
            }
            if (record.description.isEmpty()) {
                ++m_statementInvalidRowCount;
                errors << QString("Line %1: description is empty; row skipped.")
                              .arg(lineNumber);
                continue;
            }
            if (!amountOk || !qIsFinite(record.amount)) {
                ++m_statementInvalidRowCount;
                errors << QString("Line %1: unrecognized amount; row skipped.")
                              .arg(lineNumber);
                continue;
            }

            record.date = parsedDate.toString("yyyy-MM-dd");
            record.valid = true;
            const bool income = rowMeansIncome(record);
            for (const auto &transaction : m_transactions) {
                if (transaction.accountId == selectedAccountId &&
                    transaction.date == record.date &&
                    transaction.description.simplified().compare(
                        record.description.simplified(), Qt::CaseInsensitive) == 0 &&
                    qAbs(transaction.amount - qAbs(record.amount)) < 0.005 &&
                    (transaction.type == "Income") == income) {
                    record.duplicate = true;
                    record.duplicateReason = "Matches an existing transaction";
                    break;
                }
            }
            if (!record.duplicate) {
                for (const StatementRecord &previous : m_statementRows) {
                    if (previous.date == record.date &&
                        previous.description.simplified().compare(
                            record.description.simplified(), Qt::CaseInsensitive) == 0 &&
                        qAbs(qAbs(previous.amount) - qAbs(record.amount)) < 0.005 &&
                        rowMeansIncome(previous) == income) {
                        record.duplicate = true;
                        record.duplicateReason = "Repeated transaction in this file";
                        break;
                    }
                }
            }
            m_statementRows.append(record);
        }
    } else {
        errors << "Could not find a transaction header. Expected date, description, and amount columns (or debit and credit columns).";
    }
    file.close();

    if (m_statementPathLabel)
        m_statementPathLabel->setText(QFileInfo(filePath).fileName());
    if (m_statementDetectedLabel)
        m_statementDetectedLabel->setText(QString("Transactions detected: %1").arg(m_statementRows.size()));
    if (m_statementErrorLabel)
        m_statementErrorLabel->setText(QString("Parsing errors: %1").arg(errors.size()));
    if (m_statementDuplicateLabel)
        m_statementDuplicateLabel->setText(QString("Possible duplicates: %1")
            .arg(static_cast<int>(std::count_if(
                m_statementRows.cbegin(), m_statementRows.cend(),
                [](const StatementRecord &record) { return record.duplicate; }))));
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
            m_statementTable->setItem(row, 0, new QTableWidgetItem(formatDate(record.date)));
            m_statementTable->setItem(row, 1, new QTableWidgetItem(record.description));
            auto *amountItem = new QTableWidgetItem(formatMoney(record.amount));
            amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_statementTable->setItem(row, 2, amountItem);
            const QString reviewStatus = !record.duplicate ? "Ready" :
                (m_importDuplicatePolicy == "skip" ? "Will skip" : "Possible duplicate");
            m_statementTable->setItem(row, 3, new QTableWidgetItem(reviewStatus));
            m_statementTable->setItem(row, 4, new QTableWidgetItem(record.duplicate
                ? record.duplicateReason
                : QString("CSV line %1 · Account ID %2").arg(record.lineNumber).arg(selectedAccountId)));
        }
    }

    if (m_confirmImportButton)
        m_confirmImportButton->setEnabled(headerFound && !m_statementRows.isEmpty()
                                          && m_statementAccountId == selectedAccountId);

    if (!headerFound)
        QMessageBox::warning(this, "Statement format not recognised",
                             "Could not find transaction columns. The file needs date and description columns, plus an amount column or debit/credit columns.");
}

void MainWindow::confirmStatementImport()
{
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable",
            QString("Cannot import the statement because the database is unavailable: %1")
                .arg(m_databaseError));
        return;
    }

    if (m_accounts.isEmpty()
        || !m_statementAccountCombo
        || m_statementAccountCombo->currentIndex() <= 0) {
        QMessageBox::warning(
            this, "Select an account",
            "Create an account and select it before importing a statement.");
        return;
    }

    if (m_statementRows.isEmpty() || m_statementPath.isEmpty()) {
        QMessageBox::warning(
            this, "Nothing to import",
            "Choose a valid CSV statement first.");
        return;
    }

    const int selectedAccountId =
        m_statementAccountCombo->currentData().toInt();

    QString accountName;
    bool accountExists = false;

    for (const AccountRecord &account : m_accounts) {
        if (account.id == selectedAccountId) {
            accountName = account.name;
            accountExists = true;
            break;
        }
    }

    if (!accountExists || selectedAccountId != m_statementAccountId) {
        QMessageBox::warning(
            this, "Account changed",
            "The selected account differs from the account used for the preview. "
            "Choose the CSV file again and review it before importing.");
        clearStatementPreview(
            "Account changed. Choose the statement again to preview it.");
        return;
    }

    const int validRows = m_statementRows.size();
    const int possibleDuplicates = static_cast<int>(std::count_if(
        m_statementRows.cbegin(), m_statementRows.cend(),
        [](const StatementRecord &record) { return record.duplicate; }));
    const int rowsToImport = m_importDuplicatePolicy == "skip"
        ? validRows - possibleDuplicates : validRows;
    if (rowsToImport <= 0) {
        QMessageBox::information(
            this, "No new transactions",
            "Every valid row is a possible duplicate, and the current setting skips possible duplicates.");
        return;
    }
    const QString duplicateExplanation = m_importDuplicatePolicy == "skip"
        ? QString("Possible duplicate rows to skip: %1.").arg(possibleDuplicates)
        : m_importDuplicatePolicy == "import"
            ? QString("Possible duplicates to import: %1.").arg(possibleDuplicates)
            : QString("Possible duplicates are flagged for review: %1. They will be included if you proceed.")
                  .arg(possibleDuplicates);

    const auto answer = QMessageBox::question(
        this,
        "Confirm statement import",
        QString(
            "Save %1 transaction row(s) to account '%2'?\n\n"
            "%3\n"
            "Rows rejected during CSV parsing: %4\n\n"
            "Transactions will be saved to SQLite.")
            .arg(rowsToImport)
            .arg(accountName)
            .arg(duplicateExplanation)
            .arg(m_statementInvalidRowCount),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    int imported = 0;
    int failed = 0;
    int duplicatesSkipped = 0;
    QStringList saveErrors;

    for (const StatementRecord &record : m_statementRows) {
        if (record.duplicate && m_importDuplicatePolicy == "skip") {
            ++duplicatesSkipped;
            continue;
        }
        // A CSV may provide an explicit type. Recognise common income
        // and expense labels. When no recognised type is supplied,
        // use the amount's sign: positive = income, negative = expense.
        const bool isIncome = statementTypeIsIncome(record.type, record.amount);

        // TransactionService expects a positive amount in paise;
        // the separate addIncome/addExpense method determines direction.
        const auto amountPaise = static_cast<std::int64_t>(
            std::llround(qAbs(record.amount) * 100.0));

        if (amountPaise <= 0) {
            ++failed;
            saveErrors << QString("CSV line %1: amount must be greater than zero.")
                              .arg(record.lineNumber);
            continue;
        }

        int createdId = 0;
        std::string error;

        // This file format has no dedicated category column, so mark
        // these transactions as imported rather than inventing a category.
        const QString category =
            isIncome ? "Imported income" : "Imported expense";

        bool saved = false;

        if (isIncome) {
            saved = m_transactionService.addIncome(
                selectedAccountId,
                amountPaise,
                category.toStdString(),
                record.description.toStdString(),
                record.date.toStdString(),
                createdId,
                error);
        } else {
            saved = m_transactionService.addExpense(
                selectedAccountId,
                amountPaise,
                category.toStdString(),
                record.description.toStdString(),
                record.date.toStdString(),
                createdId,
                error);
        }

        if (!saved) {
            ++failed;
            saveErrors << QString("CSV line %1: %2")
                              .arg(record.lineNumber)
                              .arg(QString::fromStdString(error));
            continue;
        }

        ++imported;
    }

    // Reload data from the backend so the dashboard and tables use
    // the transactions actually saved to SQLite.
    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();

    if (m_confirmImportButton)
        m_confirmImportButton->setEnabled(false);

    QString summary = QString(
        "Import finished for '%1'.\n\n"
        "Transactions saved to SQLite: %2\n"
        "Transactions that failed to save: %3\n"
        "Possible duplicates skipped: %4\n"
        "Malformed CSV rows skipped: %5\n\n"
        "Repeated transactions are preserved as separate entries.")
        .arg(accountName)
        .arg(imported)
        .arg(failed)
        .arg(duplicatesSkipped)
        .arg(m_statementInvalidRowCount);

    if (!saveErrors.isEmpty()) {
        summary += "\n\nSave errors (first 8):\n• "
            + saveErrors.mid(0, 8).join("\n• ");

        if (saveErrors.size() > 8) {
            summary += QString("\n• ...and %1 more.")
                           .arg(saveErrors.size() - 8);
        }
    }

    if (m_statusLabel) {
        m_statusLabel->setText(
            QString("CSV import: %1 saved; %2 failed; %3 malformed row(s) skipped.")
                .arg(imported)
                .arg(failed)
                .arg(m_statementInvalidRowCount));
    }

    QMessageBox::information(this, "Import summary", summary);
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
    m_transactions.clear();
    m_accounts.clear();
    m_nextTransactionId = 1;
    m_nextAccountId = 1;

    if (!m_databaseReady) {
        if (m_statusLabel)
            m_statusLabel->setText(
                QString("Database unavailable: %1").arg(m_databaseError));
        return;
    }

    std::string error;
    const auto storedAccounts = m_accountRepository.getAll(error);

    if (!error.empty()) {
        if (m_statusLabel)
            m_statusLabel->setText(
                QString("Could not load accounts: %1")
                    .arg(QString::fromStdString(error)));
        return;
    }

    for (const auto &account : storedAccounts) {
        AccountRecord record;
        record.id = account.getId();
        record.name = QString::fromStdString(account.getName());
        record.type = QString::fromStdString(account.getType());
        record.balance =
            static_cast<double>(account.getInitialBalancePaise()) / 100.0;

        m_accounts.append(record);
        if (record.id >= m_nextAccountId)
            m_nextAccountId = record.id + 1;
    }

    refreshStatementAccountCombo();

    const auto storedTransactions =
        m_transactionService.getAllTransactions(error);

    if (!error.empty()) {
        if (m_statusLabel)
            m_statusLabel->setText(
                QString("Could not load transactions: %1")
                    .arg(QString::fromStdString(error)));
        return;
    }

    for (const auto &transaction : storedTransactions) {
        TransactionRecord record;
        record.id = transaction.getId();
        record.accountId = transaction.getAccountId();
        record.date = QString::fromStdString(transaction.getDate());
        record.description =
            QString::fromStdString(transaction.getDescription());
        record.category =
            QString::fromStdString(transaction.getCategory());
        record.type =
            transaction.getType() == TransactionType::Income
                ? "Income" : "Expense";
        record.amount =
            static_cast<double>(transaction.getAmountPaise()) / 100.0;

        m_transactions.append(record);
        if (record.id >= m_nextTransactionId)
            m_nextTransactionId = record.id + 1;
    }
    const QString search = m_transactionSearch ? m_transactionSearch->text().trimmed().toLower() : QString();
    const QString typeFilter = m_transactionTypeFilter ? m_transactionTypeFilter->currentText() : "All types";
    for (const auto &t : m_transactions) {
        if (m_activeAccountId <= 0 || t.accountId != m_activeAccountId) continue;
        if (typeFilter != "All types" && t.type != typeFilter) continue;
        if (!search.isEmpty() && !t.description.toLower().contains(search) && !t.category.toLower().contains(search)) continue;
        QString accountName = "Unknown account";
        for (const auto &a : m_accounts) if (a.id == t.accountId) accountName = a.name;
        const int row = m_transactionsTable->rowCount();
        m_transactionsTable->insertRow(row);
        m_transactionsTable->setItem(row, 0, new QTableWidgetItem(formatDate(t.date)));
        m_transactionsTable->setItem(row, 1, new QTableWidgetItem(t.description));
        m_transactionsTable->setItem(row, 2, new QTableWidgetItem(t.category));
        m_transactionsTable->setItem(row, 3, new QTableWidgetItem(t.type));
        auto *amount = new QTableWidgetItem(
            QString("%1 %2").arg(t.type == "Income" ? "+" : "−", formatMoney(t.amount)));
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
    date->setDisplayFormat(m_dateDisplayFormat);
    date->setCalendarPopup(true);
    auto *description = new QLineEdit;
    description->setPlaceholderText(income ? "e.g. Monthly salary" : "e.g. Groceries");
    auto *category = new QComboBox;
    category->setEditable(true);
    category->addItems(income ? QStringList{"Salary", "Allowance", "Interest", "Other"} :
                                 QStringList{"Food & groceries", "Transport", "Housing", "Entertainment", "Health", "Education", "Other"});
    styleComboBoxPopup(category);
    auto *amount = new QDoubleSpinBox;
    amount->setLocale(m_numberFormat == "indian"
        ? QLocale(QLocale::English, QLocale::India)
        : QLocale(QLocale::English, QLocale::UnitedStates));
    amount->setRange(0.01, 999999999.99);
    amount->setDecimals(2);
    amount->setPrefix("₹ ");
    auto *account = new QComboBox;
    for (const auto &a : m_accounts) account->addItem(a.name, a.id);
    styleComboBoxPopup(account);
    const int activeAccountIndex = account->findData(m_activeAccountId);
    if (activeAccountIndex >= 0)
        account->setCurrentIndex(activeAccountIndex);
    account->setEnabled(false);
    account->setToolTip(
        "Transactions are recorded in the Active Account selected at the top of the application.");
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
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable", m_databaseError);
        return;
    }

    const int accountId = m_activeAccountId;
    if (accountId <= 0) {
        QMessageBox::warning(this, "Select an account",
                             "Choose or create an active account before recording a transaction.");
        return;
    }
    const QString transactionDate = date->date().toString(Qt::ISODate);
    const QString transactionDescription = description->text().trimmed();
    const QString transactionCategory = category->currentText().trimmed();

    const auto amountPaise = static_cast<std::int64_t>(
        std::llround(amount->value() * 100.0));

    int createdId = 0;
    std::string error;
    bool saved = false;

    if (income) {
        saved = m_transactionService.addIncome(
            accountId, amountPaise,
            transactionCategory.toStdString(),
            transactionDescription.toStdString(),
            transactionDate.toStdString(), createdId, error);
    } else {
        saved = m_transactionService.addExpense(
            accountId, amountPaise,
            transactionCategory.toStdString(),
            transactionDescription.toStdString(),
            transactionDate.toStdString(), createdId, error);
    }

    if (!saved) {
        QMessageBox::warning(
            this, "Transaction not saved",
            QString::fromStdString(error));
        return;
    }

    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();

    QMessageBox::information(
        this, "Transaction saved",
        QString("%1 saved successfully. Transaction ID: %2")
            .arg(income ? "Income" : "Expense")
            .arg(createdId));
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
    auto *date = new QDateEdit(QDate::fromString(t.date, Qt::ISODate));
    date->setDisplayFormat(m_dateDisplayFormat);
    date->setCalendarPopup(true);
    auto *description = new QLineEdit(t.description);
    auto *category = new QLineEdit(t.category);
    auto *amount = new QDoubleSpinBox; amount->setLocale(m_numberFormat == "indian" ? QLocale(QLocale::English, QLocale::India) : QLocale(QLocale::English, QLocale::UnitedStates)); amount->setRange(0.01, 999999999.99); amount->setDecimals(2); amount->setPrefix("₹ "); amount->setValue(t.amount);
    auto *account = new QComboBox;
    for (const auto &a : m_accounts) account->addItem(a.name, a.id);
    styleComboBoxPopup(account);
    account->setCurrentIndex(qMax(0, account->findData(m_activeAccountId)));
    account->setEnabled(false);
    account->setToolTip(
        "The transaction belongs to the Active Account selected at the top of the application.");
    form->addRow("Date", date); form->addRow("Description", description); form->addRow("Category", category); form->addRow("Amount", amount); form->addRow("Account", account);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel); form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    if (description->text().trimmed().isEmpty() || category->text().trimmed().isEmpty()) { QMessageBox::warning(this, "Invalid transaction", "Description and category are required."); return; }
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable", m_databaseError);
        return;
    }

    const QString updatedDate = date->date().toString(Qt::ISODate);
    const QString updatedDescription = description->text().trimmed();
    const QString updatedCategory = category->text().trimmed();
    const int updatedAccountId = m_activeAccountId;

    const auto updatedAmountPaise = static_cast<std::int64_t>(
        std::llround(amount->value() * 100.0));

    const TransactionType updatedType =
        t.type == "Income"
            ? TransactionType::Income
            : TransactionType::Expense;

    try {
        Transaction updatedTransaction(
            id, updatedAccountId, updatedAmountPaise, updatedType,
            updatedCategory.toStdString(),
            updatedDescription.toStdString(),
            updatedDate.toStdString());

        std::string error;

        if (!m_transactionService.updateTransaction(
                updatedTransaction, error)) {
            QMessageBox::warning(
                this, "Transaction not updated",
                QString::fromStdString(error));
            return;
        }
    } catch (const std::exception &exception) {
        QMessageBox::warning(
            this, "Invalid transaction",
            QString::fromStdString(exception.what()));
        return;
    }

    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();

    QMessageBox::information(
        this, "Transaction updated",
        "Transaction changes were saved to the database.");
}

void MainWindow::deleteTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) return;
    const int id = m_transactionsTable->item(row, 0)->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "Delete transaction", "Delete the selected transaction?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable", m_databaseError);
        return;
    }

    std::string error;

    if (!m_transactionService.deleteTransaction(id, error)) {
        QMessageBox::warning(
            this, "Transaction not deleted",
            QString::fromStdString(error));
        return;
    }

    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();

    QMessageBox::information(
        this, "Transaction deleted",
        "The transaction was deleted from the database.");
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
    m_budgetsTable = new QTableWidget(0, 7); m_budgetsTable->setObjectName("accountsTable");
    m_budgetsTable->setHorizontalHeaderLabels({"Account", "Month", "Category", "Budget limit", "Spent", "Remaining", "Status"});
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
    if (!m_budgetsTable)
        return;

    m_budgetsTable->setRowCount(0);
    m_budgets.clear();
    m_nextBudgetId = 1;

    if (!m_databaseReady) {
        if (m_statusLabel)
            m_statusLabel->setText(
                QString("Database unavailable: %1").arg(m_databaseError));
        return;
    }

    std::string error;
    const auto statuses = m_budgetService.getAllBudgetStatuses(error);

    if (!error.empty()) {
        if (m_statusLabel)
            m_statusLabel->setText(
                QString("Could not load budgets: %1")
                    .arg(QString::fromStdString(error)));
        return;
    }

    for (const auto &status : statuses) {
        const Budget &budget = status.budget;

        BudgetRecord record;
        record.id = budget.getId();
        record.accountId = budget.getAccountId();
        record.accountName = record.accountId == 0
            ? "Unassigned"
            : QString("Unknown account (%1)").arg(record.accountId);
        for (const auto &account : m_accounts) {
            if (account.id == record.accountId) {
                record.accountName = account.name;
                break;
            }
        }
        record.category = QString::fromStdString(budget.getCategory());
        record.limit =
            static_cast<double>(budget.getLimitPaise()) / 100.0;
        record.month = QString("%1-%2")
            .arg(budget.getYear(), 4, 10, QChar('0'))
            .arg(budget.getMonth(), 2, 10, QChar('0'));

        m_budgets.append(record);
        if (record.id >= m_nextBudgetId)
            m_nextBudgetId = record.id + 1;

        // Keep unassigned legacy budgets visible so the user can assign them,
        // but show other accounts' assigned budgets only when that account is active.
        if (record.accountId > 0 && record.accountId != m_activeAccountId)
            continue;

        const int row = m_budgetsTable->rowCount();
        m_budgetsTable->insertRow(row);

        const double spent =
            static_cast<double>(status.spentPaise) / 100.0;
        const double remaining =
            static_cast<double>(status.remainingPaise) / 100.0;

        m_budgetsTable->setItem(
            row, 0, new QTableWidgetItem(record.accountName));
        m_budgetsTable->setItem(
            row, 1, new QTableWidgetItem(record.month));
        m_budgetsTable->setItem(
            row, 2, new QTableWidgetItem(record.category));
        m_budgetsTable->setItem(
            row, 3, new QTableWidgetItem(
                formatMoney(record.limit)));
        m_budgetsTable->setItem(
            row, 4, new QTableWidgetItem(
                formatMoney(spent)));
        m_budgetsTable->setItem(
            row, 5, new QTableWidgetItem(
                formatMoney(remaining)));

        const QString budgetStatus = status.overBudget
            ? "OVER BUDGET"
            : (spent >= record.limit * 0.8 ? "Near limit" : "On track");

        m_budgetsTable->setItem(
            row, 6, new QTableWidgetItem(budgetStatus));
        m_budgetsTable->item(row, 0)->setData(Qt::UserRole, record.id);

        if (status.overBudget) {
            for (int column = 0; column < 7; ++column)
                m_budgetsTable->item(row, column)
                    ->setBackground(QColor("#fff0f0"));
        }
    }

    if (m_editBudgetButton)
        m_editBudgetButton->setEnabled(false);
    if (m_deleteBudgetButton)
        m_deleteBudgetButton->setEnabled(false);
}

void MainWindow::addBudget()
{
    if (m_accounts.isEmpty()) {
        QMessageBox::information(
            this, "Create an account first",
            "Create an account before setting a budget.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Create budget");
    auto *form = new QFormLayout(&dialog);

    auto *account = new QComboBox;
    account->addItem("Select an account...", 0);
    for (const auto &item : m_accounts)
        account->addItem(item.name, item.id);
    styleComboBoxPopup(account);
    const int activeAccountIndex = account->findData(m_activeAccountId);
    if (activeAccountIndex >= 0)
        account->setCurrentIndex(activeAccountIndex);
    account->setEnabled(false);
    account->setToolTip(
        "The budget is created for the Active Account selected at the top of the application.");

    auto *category = new QComboBox;
    category->setEditable(true);
    category->addItems({
        "Food & groceries", "Transport", "Housing", "Entertainment",
        "Health", "Education", "Utilities", "Other"});
    styleComboBoxPopup(category);

    auto *limit = new QDoubleSpinBox;
    limit->setLocale(m_numberFormat == "indian"
        ? QLocale(QLocale::English, QLocale::India)
        : QLocale(QLocale::English, QLocale::UnitedStates));
    limit->setRange(0.01, 999999999.99);
    limit->setDecimals(2);
    limit->setPrefix("₹ ");

    auto *month = new QDateEdit(QDate::currentDate());
    month->setDisplayFormat("MMMM yyyy");
    month->setCalendarPopup(true);

    form->addRow("Account", account);
    form->addRow("Category", category);
    form->addRow("Monthly limit", limit);
    form->addRow("Month", month);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted,
            &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const int accountId = m_activeAccountId;
    if (accountId <= 0) {
        QMessageBox::warning(
            this, "Select an account",
            "Choose or create an active account before creating a budget.");
        return;
    }
    if (category->currentText().trimmed().isEmpty() || limit->value() <= 0) {
        QMessageBox::warning(
            this, "Invalid budget", "Enter a category and a positive limit.");
        return;
    }
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable", m_databaseError);
        return;
    }

    const QDate selectedMonth = month->date();
    const QString selectedCategory = category->currentText().trimmed();
    const auto limitPaise = static_cast<std::int64_t>(
        std::llround(limit->value() * 100.0));

    int createdBudgetId = 0;
    std::string error;
    if (!m_budgetService.addBudget(
            accountId,
            selectedCategory.toStdString(),
            selectedMonth.month(),
            selectedMonth.year(),
            limitPaise,
            createdBudgetId,
            error)) {
        QMessageBox::warning(
            this, "Budget not saved", QString::fromStdString(error));
        return;
    }

    refreshBudgetsTable();
    refreshDashboard();
    QMessageBox::information(
        this, "Budget created",
        QString("Budget saved successfully (ID %1).")
            .arg(createdBudgetId));
}

void MainWindow::editBudget()
{
    const int row = m_budgetsTable ? m_budgetsTable->currentRow() : -1;
    if (row < 0)
        return;

    const int id = m_budgetsTable->item(row, 0)->data(Qt::UserRole).toInt();
    int index = -1;
    for (int i = 0; i < m_budgets.size(); ++i) {
        if (m_budgets[i].id == id) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return;

    const BudgetRecord original = m_budgets[index];
    QDialog dialog(this);
    dialog.setWindowTitle("Edit budget");
    auto *form = new QFormLayout(&dialog);

    if (m_activeAccountId <= 0) {
        QMessageBox::information(
            this, "Create an account first",
            "Create an account and select it as the active account before editing a budget.");
        return;
    }
    auto *account = new QComboBox;
    for (const auto &item : m_accounts)
        account->addItem(item.name, item.id);
    styleComboBoxPopup(account);
    const int accountIndex = account->findData(m_activeAccountId);
    if (accountIndex >= 0)
        account->setCurrentIndex(accountIndex);
    account->setEnabled(false);
    account->setToolTip(
        "Budget ownership follows the Active Account selector at the top of the application.");

    auto *category = new QLineEdit(original.category);
    auto *limit = new QDoubleSpinBox;
    limit->setLocale(m_numberFormat == "indian"
        ? QLocale(QLocale::English, QLocale::India)
        : QLocale(QLocale::English, QLocale::UnitedStates));
    limit->setRange(0.01, 999999999.99);
    limit->setDecimals(2);
    limit->setPrefix("₹ ");
    limit->setValue(original.limit);

    auto *month = new QDateEdit(
        QDate::fromString(original.month + "-01", Qt::ISODate));
    month->setDisplayFormat("MMMM yyyy");
    month->setCalendarPopup(true);

    form->addRow("Account", account);
    form->addRow("Category", category);
    form->addRow("Monthly limit", limit);
    form->addRow("Month", month);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted,
            &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    if (category->text().trimmed().isEmpty()) {
        QMessageBox::warning(
            this, "Invalid budget", "Category is required.");
        return;
    }
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable", m_databaseError);
        return;
    }

    const QDate selectedMonth = month->date();
    const auto limitPaise = static_cast<std::int64_t>(
        std::llround(limit->value() * 100.0));
    const int accountId = account->currentData().toInt();

    try {
        Budget updatedBudget(
            id,
            accountId,
            category->text().trimmed().toStdString(),
            selectedMonth.month(),
            selectedMonth.year(),
            limitPaise);

        std::string error;
        if (!m_budgetService.updateBudget(updatedBudget, error)) {
            QMessageBox::warning(
                this, "Budget not updated", QString::fromStdString(error));
            return;
        }
    } catch (const std::exception &exception) {
        QMessageBox::warning(
            this, "Invalid budget", QString::fromStdString(exception.what()));
        return;
    }

    refreshBudgetsTable();
    refreshDashboard();
    QMessageBox::information(
        this, "Budget updated", "Budget changes were saved to SQLite.");
}

void MainWindow::deleteBudget()
{
    int row=m_budgetsTable?m_budgetsTable->currentRow():-1; if(row<0)return;
    int id=m_budgetsTable->item(row,0)->data(Qt::UserRole).toInt();
    if(QMessageBox::question(this,"Delete budget","Delete this budget?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes)return;
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Database unavailable", m_databaseError);
        return;
    }

    std::string error;

    if (!m_budgetService.deleteBudget(id, error)) {
        QMessageBox::warning(
            this, "Budget not deleted",
            QString::fromStdString(error));
        return;
    }

    refreshBudgetsTable();
    refreshDashboard();

    QMessageBox::information(
        this, "Budget deleted",
        "The budget was deleted from SQLite.");
}
QWidget *MainWindow::buildReportsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    auto *controls = new QFrame;
    controls->setObjectName("contentPanel");
    auto *controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(18, 14, 18, 14);
    controlsLayout->setSpacing(12);
    m_reportTypeCombo = new QComboBox;
    m_reportTypeCombo->addItems({"Income and expenses", "Category spending",
                                 "Budget status", "Account balances"});
    m_reportPeriodCombo = new QComboBox;
    m_reportPeriodCombo->addItems({"This month", "Last month", "All time"});
    if (m_reportPeriodCombo->findText(m_defaultReportPeriod) >= 0)
        m_reportPeriodCombo->setCurrentText(m_defaultReportPeriod);
    auto *run = new QPushButton("Generate report");
    run->setObjectName("primaryActionButton");
    controlsLayout->addWidget(makeLabel("REPORT", "metricCaption"));
    controlsLayout->addWidget(m_reportTypeCombo, 1);
    controlsLayout->addWidget(makeLabel("PERIOD", "metricCaption"));
    controlsLayout->addWidget(m_reportPeriodCombo, 1);
    controlsLayout->addWidget(run);
    layout->addWidget(controls);

    m_reportSummary = makeLabel(
        "Choose a report and period to explore your account activity.", "panelHint");
    layout->addWidget(m_reportSummary);

    auto *chartRow = new QHBoxLayout;
    chartRow->setSpacing(14);
    auto *donutPanel = new QFrame;
    donutPanel->setObjectName("contentPanel");
    auto *donutLayout = new QVBoxLayout(donutPanel);
    donutLayout->setContentsMargins(16, 14, 16, 12);
    m_reportDonutTitle = makeLabel("Expense breakdown", "panelTitle");
    donutLayout->addWidget(m_reportDonutTitle);
    m_reportDonutChart = new DonutChartWidget;
    m_reportDonutChart->setDarkMode(m_themePreference == "dark");
    donutLayout->addWidget(m_reportDonutChart, 1);
    auto *barPanel = new QFrame;
    barPanel->setObjectName("contentPanel");
    auto *barLayout = new QVBoxLayout(barPanel);
    barLayout->setContentsMargins(16, 14, 16, 12);
    m_reportBarTitle = makeLabel("Income vs expenses", "panelTitle");
    barLayout->addWidget(m_reportBarTitle);
    m_reportBarChart = new HorizontalBarChartWidget;
    m_reportBarChart->setDarkMode(m_themePreference == "dark");
    m_reportBarChart->setIndianNumberFormat(m_numberFormat == "indian");
    barLayout->addWidget(m_reportBarChart, 1);
    chartRow->addWidget(donutPanel, 1);
    chartRow->addWidget(barPanel, 1);
    layout->addLayout(chartRow);

    m_reportBudgetPanel = new QFrame;
    m_reportBudgetPanel->setObjectName("contentPanel");
    auto *budgetLayout = new QVBoxLayout(m_reportBudgetPanel);
    budgetLayout->setContentsMargins(16, 14, 16, 14);
    budgetLayout->addWidget(makeLabel("Budget remaining", "panelTitle"));
    auto *budgetScroll = new QScrollArea;
    budgetScroll->setWidgetResizable(true);
    budgetScroll->setFrameShape(QFrame::NoFrame);
    budgetScroll->setMaximumHeight(190);
    auto *budgetRows = new QWidget;
    m_reportBudgetRowsLayout = new QVBoxLayout(budgetRows);
    m_reportBudgetRowsLayout->setContentsMargins(0, 0, 0, 0);
    m_reportBudgetRowsLayout->setSpacing(10);
    budgetScroll->setWidget(budgetRows);
    budgetLayout->addWidget(budgetScroll);
    m_reportBudgetPanel->hide();
    layout->addWidget(m_reportBudgetPanel);

    m_reportTable = new QTableWidget(0, 3);
    m_reportTable->setObjectName("accountsTable");
    m_reportTable->setHorizontalHeaderLabels({"Item", "Amount", "Details"});
    m_reportTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_reportTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reportTable->setAlternatingRowColors(true);
    m_reportTable->verticalHeader()->setVisible(false);
    layout->addWidget(m_reportTable, 1);

    connect(run, &QPushButton::clicked, this, &MainWindow::runReport);
    connect(m_reportPeriodCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index < 0 || !m_reportPeriodCombo)
            return;
        m_defaultReportPeriod = m_reportPeriodCombo->currentText();
        QSettings().setValue("settings/defaultReportPeriod", m_defaultReportPeriod);
        if (m_settingsReportPeriodCombo)
            m_settingsReportPeriodCombo->setCurrentText(m_defaultReportPeriod);
        if (m_reportWasGenerated)
            runReport();
    });
    return page;
}

void MainWindow::runReport()
{
    if (!m_reportTable || !m_reportTypeCombo || !m_reportPeriodCombo)
        return;
    m_reportTable->setRowCount(0);
    m_reportWasGenerated = true;
    const QString type = m_reportTypeCombo->currentText();
    const QString period = m_reportPeriodCombo->currentText();
    const QDate today = QDate::currentDate();
    const QString thisMonth = today.toString("yyyy-MM");
    const QString lastMonth = today.addMonths(-1).toString("yyyy-MM");
    if (m_reportDonutTitle) {
        m_reportDonutTitle->setText(type == "Budget status"
            ? "Budget used vs available" :
            type == "Category spending" ? "Expense by category" :
                                           "Expense breakdown");
    }
    if (m_reportBarTitle) {
        m_reportBarTitle->setText(type == "Budget status"
            ? "Spending by budget" :
            type == "Category spending" ? "Top expense categories" :
            type == "Account balances" ? "Account balance" :
                                          "Income vs expenses");
    }
    const QDate lastMonthEnd = QDate::fromString(lastMonth + "-01", "yyyy-MM-dd")
                                   .addMonths(1).addDays(-1);
    auto inPeriod = [&](const QString &date) {
        return period == "All time" ||
               (period == "This month" && date.left(7) == thisMonth) ||
               (period == "Last month" && date.left(7) == lastMonth);
    };
    auto add = [&](const QString &name, double amount, const QString &detail) {
        const int row = m_reportTable->rowCount();
        m_reportTable->insertRow(row);
        m_reportTable->setItem(row, 0, new QTableWidgetItem(name));
        m_reportTable->setItem(row, 1, new QTableWidgetItem(formatMoney(amount)));
        m_reportTable->setItem(row, 2, new QTableWidgetItem(detail));
    };
    double income = 0.0, expense = 0.0;
    QMap<QString, double> categories;
    int activeTransactionCount = 0;
    for (const auto &t : m_transactions) {
        if (t.accountId != m_activeAccountId || !inPeriod(t.date))
            continue;
        ++activeTransactionCount;
        if (t.type == "Income")
            income += t.amount;
        else {
            expense += t.amount;
            categories[t.category] += t.amount;
        }
    }

    QVector<QPair<QString, double>> categoryValues;
    for (auto it = categories.cbegin(); it != categories.cend(); ++it)
        categoryValues.append(qMakePair(it.key(), it.value()));
    std::sort(categoryValues.begin(), categoryValues.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    QVector<QPair<QString, double>> donutValues;
    double otherCategoryTotal = 0.0;
    for (int i = 0; i < categoryValues.size(); ++i) {
        if (i < 5)
            donutValues.append(categoryValues.at(i));
        else
            otherCategoryTotal += categoryValues.at(i).second;
    }
    if (otherCategoryTotal > 0.0)
        donutValues.append(qMakePair(QString("Other"), otherCategoryTotal));

    QVector<QPair<QString, double>> barValues;
    if (type == "Category spending") {
        barValues = categoryValues;
        for (const auto &entry : categoryValues)
            add(entry.first, entry.second, "Expense category");
    } else if (type == "Budget status") {
        double totalSpent = 0.0;
        double totalRemaining = 0.0;
        for (const auto &b : m_budgets) {
            if (m_activeAccountId <= 0 || b.accountId != m_activeAccountId)
                continue;
            if (period != "All time" &&
                b.month != (period == "This month" ? thisMonth : lastMonth))
                continue;

            double spent = 0.0;
            for (const auto &t : m_transactions) {
                if (t.accountId == b.accountId && t.type == "Expense" &&
                    t.category.compare(b.category, Qt::CaseInsensitive) == 0 &&
                    t.date.left(7) == b.month)
                    spent += t.amount;
            }
            const double remaining = b.limit - spent;
            const QString label = QString("%1 · %2 · %3")
                .arg(b.category, b.accountName, b.month);
            const QString status = spent > b.limit ? "Over budget" :
                (spent >= b.limit * 0.8 ? "Near limit" : "On track");
            add(QString("%1 — %2").arg(b.accountName, b.category), spent,
                QString("%1 · Limit %2 · Remaining %3")
                    .arg(status, formatMoney(b.limit), formatMoney(remaining)));
            barValues.append(qMakePair(label, spent));
            totalSpent += spent;
            totalRemaining += qMax(0.0, remaining);

        }
        if (m_reportDonutChart) {
            QVector<QPair<QString, double>> budgetSplit;
            if (totalSpent > 0.0)
                budgetSplit.append(qMakePair(QString("Spent"), totalSpent));
            if (totalRemaining > 0.0)
                budgetSplit.append(qMakePair(QString("Still available"), totalRemaining));
            m_reportDonutChart->setValues(budgetSplit);
        }
    } else {
        if (type == "Income and expenses") {
            add("Income", income, period);
            add("Expenses", expense, period);
            add("Net savings", income - expense, "Income minus expenses");
        } else {
            const QDate cutoff = period == "Last month"
                ? lastMonthEnd : today;
            for (const auto &a : m_accounts) {
                if (a.id != m_activeAccountId)
                    continue;
                double balance = a.balance;
                for (const auto &t : m_transactions) {
                    if (t.accountId == a.id &&
                        QDate::fromString(t.date, "yyyy-MM-dd") <= cutoff)
                        balance += (t.type == "Income" ? t.amount : -t.amount);
                }
                add(a.name, balance,
                    period == "All time" ? "Current balance" :
                    QString("Balance at %1").arg(formatDate(cutoff.toString(Qt::ISODate))));
                barValues.append(qMakePair(a.name, balance));
            }
        }
    }

    if (type != "Budget status" && m_reportDonutChart)
        m_reportDonutChart->setValues(donutValues);
    if (type == "Income and expenses") {
        barValues = {{"Income", income}, {"Expenses", expense}};
    }
    std::sort(barValues.begin(), barValues.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if (m_reportBarChart) {
        m_reportBarChart->setIndianNumberFormat(m_numberFormat == "indian");
        m_reportBarChart->setValues(barValues);
    }

    if (m_reportBudgetRowsLayout) {
        while (QLayoutItem *item = m_reportBudgetRowsLayout->takeAt(0)) {
            if (QWidget *widget = item->widget())
                widget->deleteLater();
            delete item;
        }
    }
    // Rebuild the budget rows after clearing rows from the previous report.
    if (type == "Budget status") {
        for (const auto &b : m_budgets) {
            if (m_activeAccountId <= 0 || b.accountId != m_activeAccountId ||
                (period != "All time" &&
                 b.month != (period == "This month" ? thisMonth : lastMonth)))
                continue;
            double spent = 0.0;
            for (const auto &t : m_transactions) {
                if (t.accountId == b.accountId && t.type == "Expense" &&
                    t.category.compare(b.category, Qt::CaseInsensitive) == 0 &&
                    t.date.left(7) == b.month)
                    spent += t.amount;
            }
            auto *row = new QWidget(m_reportBudgetPanel);
            auto *rowLayout = new QVBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(4);
            auto *labels = new QHBoxLayout;
            const double remaining = b.limit - spent;
            const QString remainingText = remaining >= 0.0
                ? QString("%1 spent  ·  %2 left")
                      .arg(formatMoney(spent), formatMoney(remaining))
                : QString("%1 spent  ·  Over by %2")
                      .arg(formatMoney(spent), formatMoney(-remaining));
            labels->addWidget(makeLabel(QString("%1 · %2").arg(b.category, b.month),
                                        "budgetRowTitle"), 1);
            labels->addWidget(makeLabel(remainingText, "budgetRowValue"),
                              0, Qt::AlignRight);
            auto *progress = new QProgressBar;
            progress->setRange(0, 100);
            progress->setValue(b.limit > 0.0
                ? qBound(0, qRound(spent / b.limit * 100.0), 100) : 0);
            progress->setFormat("%p% used");
            rowLayout->addLayout(labels);
            rowLayout->addWidget(progress);
            m_reportBudgetRowsLayout->addWidget(row);
        }
        if (m_reportBudgetRowsLayout->count() == 0)
            m_reportBudgetRowsLayout->addWidget(makeLabel(
                "No budgets match this account and period.", "panelHint"));
    }
    if (m_reportBudgetPanel)
        m_reportBudgetPanel->setVisible(type == "Budget status");

    if (m_reportTable->rowCount() == 0) {
        add("No matching data", 0.0,
            "Add transactions or budgets for this account and period.");
        m_reportTable->item(0, 1)->setText("—");
    }
    m_reportSummary->setText(
        QString("%1 · %2 · %3 transaction(s) in this period for the active account")
            .arg(type, period).arg(activeTransactionCount));
}

QString MainWindow::formatMoney(double amount) const
{
    return formatCurrency(amount, m_numberFormat == "indian");
}

QString MainWindow::formatDate(const QString &isoDate) const
{
    const QDate date = QDate::fromString(isoDate, Qt::ISODate);
    return date.isValid() ? date.toString(m_dateDisplayFormat) : isoDate;
}

void MainWindow::applyDisplayPreferences()
{
    if (m_settingsThemeCombo)
        m_themePreference = m_settingsThemeCombo->currentData().toString();
    if (m_settingsDateFormatCombo)
        m_dateDisplayFormat = m_settingsDateFormatCombo->currentData().toString();
    if (m_settingsNumberFormatCombo)
        m_numberFormat = m_settingsNumberFormatCombo->currentData().toString();
    if (m_settingsReportPeriodCombo)
        m_defaultReportPeriod = m_settingsReportPeriodCombo->currentData().toString();
    if (m_settingsDuplicatePolicyCombo)
        m_importDuplicatePolicy = m_settingsDuplicatePolicyCombo->currentData().toString();

    QSettings settings;
    settings.setValue("settings/theme", m_themePreference);
    settings.setValue("settings/dateFormat", m_dateDisplayFormat);
    settings.setValue("settings/numberFormat", m_numberFormat);
    settings.setValue("settings/defaultReportPeriod", m_defaultReportPeriod);
    settings.setValue("settings/importDuplicatePolicy", m_importDuplicatePolicy);

    if (m_reportPeriodCombo && m_reportPeriodCombo->currentText() != m_defaultReportPeriod)
        m_reportPeriodCombo->setCurrentText(m_defaultReportPeriod);

    applyStyles();
    if (m_reportDonutChart)
        m_reportDonutChart->setDarkMode(m_themePreference == "dark");
    if (m_reportBarChart) {
        m_reportBarChart->setDarkMode(m_themePreference == "dark");
        m_reportBarChart->setIndianNumberFormat(m_numberFormat == "indian");
    }
    refreshAccountsTable();
    refreshTransactionsTable();
    refreshBudgetsTable();
    refreshDashboard();
    if (m_reportWasGenerated)
        runReport();
    if (m_statementTable) {
        for (int row = 0; row < m_statementRows.size(); ++row) {
            if (m_statementTable->item(row, 0))
                m_statementTable->item(row, 0)->setText(formatDate(m_statementRows.at(row).date));
            if (m_statementTable->item(row, 2))
                m_statementTable->item(row, 2)->setText(formatMoney(m_statementRows.at(row).amount));
            if (m_statementTable->item(row, 3)) {
                const StatementRecord &record = m_statementRows.at(row);
                m_statementTable->item(row, 3)->setText(!record.duplicate ? "Ready" :
                    (m_importDuplicatePolicy == "skip" ? "Will skip" : "Possible duplicate"));
            }
        }
    }
    if (m_statusLabel)
        m_statusLabel->setText("Settings saved on this device");
}

QWidget *MainWindow::buildSettingsPage()
{
    auto *page = new QWidget;
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 4, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setObjectName("settingsScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto *intro = new QFrame;
    intro->setObjectName("contentPanel");
    auto *introLayout = new QVBoxLayout(intro);
    introLayout->setContentsMargins(20, 18, 20, 18);
    introLayout->addWidget(makeLabel("Make PFBM yours", "panelTitle"));
    auto *introText = makeLabel(
        "Choose how the app looks and displays financial information. Changes are saved automatically on this device.",
        "panelHint");
    introText->setWordWrap(true);
    introLayout->addWidget(introText);
    layout->addWidget(intro);

    m_settingsThemeCombo = new QComboBox;
    m_settingsThemeCombo->addItem("Light", "light");
    m_settingsThemeCombo->addItem("Dark", "dark");
    m_settingsThemeCombo->setCurrentIndex(
        qMax(0, m_settingsThemeCombo->findData(m_themePreference)));

    m_settingsDateFormatCombo = new QComboBox;
    m_settingsDateFormatCombo->addItem("DD/MM/YYYY  ·  28/10/2026", "dd/MM/yyyy");
    m_settingsDateFormatCombo->addItem("YYYY-MM-DD  ·  2026-10-28", "yyyy-MM-dd");
    m_settingsDateFormatCombo->addItem("D Mon YYYY  ·  28 Oct 2026", "d MMM yyyy");
    m_settingsDateFormatCombo->setCurrentIndex(
        qMax(0, m_settingsDateFormatCombo->findData(m_dateDisplayFormat)));

    m_settingsNumberFormatCombo = new QComboBox;
    m_settingsNumberFormatCombo->addItem("Indian grouping  ·  ₹ 1,23,456.78", "indian");
    m_settingsNumberFormatCombo->addItem("Western grouping  ·  ₹ 123,456.78", "western");
    m_settingsNumberFormatCombo->setCurrentIndex(
        qMax(0, m_settingsNumberFormatCombo->findData(m_numberFormat)));

    m_settingsReportPeriodCombo = new QComboBox;
    m_settingsReportPeriodCombo->addItems({"This month", "Last month", "All time"});
    m_settingsReportPeriodCombo->setCurrentText(m_defaultReportPeriod);

    m_settingsDuplicatePolicyCombo = new QComboBox;
    m_settingsDuplicatePolicyCombo->addItem("Flag for review", "flag");
    m_settingsDuplicatePolicyCombo->addItem("Skip possible duplicates", "skip");
    m_settingsDuplicatePolicyCombo->addItem("Import all rows", "import");
    m_settingsDuplicatePolicyCombo->setCurrentIndex(
        qMax(0, m_settingsDuplicatePolicyCombo->findData(m_importDuplicatePolicy)));

    auto addSettingCard = [&](const QString &title, const QString &description,
                              const QString &fieldLabel, QComboBox *control) {
        auto *card = new QFrame;
        card->setObjectName("contentPanel");
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(18, 16, 18, 16);
        cardLayout->setSpacing(6);
        cardLayout->addWidget(makeLabel(title, "panelTitle"));
        auto *hint = makeLabel(description, "panelHint");
        hint->setWordWrap(true);
        cardLayout->addWidget(hint);
        auto *row = new QHBoxLayout;
        row->addWidget(makeLabel(fieldLabel, "budgetRowTitle"), 1);
        control->setMinimumWidth(250);
        row->addWidget(control);
        cardLayout->addLayout(row);
        layout->addWidget(card);
    };

    addSettingCard("Appearance", "Switch between a bright workspace and a dark interface.",
                   "Application theme", m_settingsThemeCombo);
    addSettingCard("Date and number formats",
                   "These choices affect displayed dates and rupee amounts; saved transaction dates and values are unchanged.",
                   "Date format", m_settingsDateFormatCombo);
    addSettingCard("Number grouping", "Choose Indian or Western digit grouping for displayed amounts.",
                   "Amount format", m_settingsNumberFormatCombo);
    addSettingCard("Reports", "Choose the period selected by default when you open Reports.",
                   "Default report period", m_settingsReportPeriodCombo);
    addSettingCard("CSV imports",
                   "Possible duplicates match a date, description, amount, and transaction direction. Review flagged rows before confirming; repeated real purchases can still be legitimate.",
                   "Duplicate handling", m_settingsDuplicatePolicyCombo);

    auto *storageNote = makeLabel(
        "Your active account is already remembered automatically. Account data stays in the local SQLite database. Backups and restores are managed on the separate Backup & restore page.",
        "panelHint");
    storageNote->setWordWrap(true);
    layout->addWidget(storageNote);

    auto *about = new QFrame;
    about->setObjectName("contentPanel");
    auto *aboutLayout = new QVBoxLayout(about);
    aboutLayout->setContentsMargins(18, 14, 18, 14);
    aboutLayout->addWidget(makeLabel("About PFBM", "panelTitle"));
    aboutLayout->addWidget(makeLabel(
        "Personal Finance & Budgeting Dashboard · local SQLite storage · JSON backups",
        "panelHint"));
    layout->addWidget(about);
    layout->addStretch();

    auto preferencesChanged = [this](int) { applyDisplayPreferences(); };
    connect(m_settingsThemeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, preferencesChanged);
    connect(m_settingsDateFormatCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, preferencesChanged);
    connect(m_settingsNumberFormatCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, preferencesChanged);
    connect(m_settingsReportPeriodCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, preferencesChanged);
    connect(m_settingsDuplicatePolicyCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, preferencesChanged);
    scroll->setWidget(content);
    pageLayout->addWidget(scroll);
    return page;
}

void MainWindow::refreshDashboard()
{
    // Refresh dashboard values without changing the user's current page.
    if (!m_pages || m_pages->count() == 0)
        return;

    QWidget *oldDashboard = m_pages->widget(0);
    QWidget *currentPage = m_pages->currentWidget();
    const bool dashboardWasCurrent = (currentPage == oldDashboard);
    QWidget *freshDashboard = buildDashboardPage();

    // QStackedWidget may switch to the next page when its current widget is
    // removed. Move temporarily to another page to avoid leaving the
    // Dashboard navigation button selected while Transactions is displayed.
    if (dashboardWasCurrent && m_pages->count() > 1)
        m_pages->setCurrentIndex(1);

    m_pages->removeWidget(oldDashboard);
    oldDashboard->deleteLater();
    m_pages->insertWidget(0, freshDashboard);

    // Restore the exact page that was selected before the dashboard refresh.
    if (dashboardWasCurrent)
        m_pages->setCurrentWidget(freshDashboard);
    else if (currentPage)
        m_pages->setCurrentWidget(currentPage);
}

QWidget *MainWindow::buildUtilitiesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(16);

    auto *intro = new QFrame;
    intro->setObjectName("contentPanel");
    auto *introLayout = new QVBoxLayout(intro);
    introLayout->setContentsMargins(20, 18, 20, 18);
    introLayout->addWidget(makeLabel("Protect your financial data", "panelTitle"));
    auto *description = makeLabel(
        "Create a portable JSON backup of your SQLite data or restore a previous backup. Restoring replaces the current database after confirmation.",
        "panelHint");
    description->setWordWrap(true);
    introLayout->addWidget(description);
    layout->addWidget(intro);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(14);
    auto makeActionCard = [&](const QString &title, const QString &descriptionText,
                              const QString &buttonText, const QString &objectName) {
        auto *card = new QFrame;
        card->setObjectName("contentPanel");
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);
        cardLayout->setSpacing(10);
        cardLayout->addWidget(makeLabel(title, "panelTitle"));
        auto *detail = makeLabel(descriptionText, "panelHint");
        detail->setWordWrap(true);
        cardLayout->addWidget(detail);
        auto *button = new QPushButton(buttonText);
        button->setObjectName(objectName);
        button->setCursor(Qt::PointingHandCursor);
        cardLayout->addWidget(button, 0, Qt::AlignLeft);
        cardLayout->addStretch();
        actions->addWidget(card, 1);
        return button;
    };

    auto *backup = makeActionCard(
        "Create a backup", "Save all accounts, transactions, and budgets to a JSON file.",
        "Create backup", "primaryActionButton");
    auto *restore = makeActionCard(
        "Restore a backup", "Load a PFBM JSON backup. This replaces the current accounts, transactions, and budgets.",
        "Choose backup to restore", "dangerActionButton");
    layout->addLayout(actions);
    layout->addStretch();
    connect(backup, &QPushButton::clicked, this, &MainWindow::backupData);
    connect(restore, &QPushButton::clicked, this, &MainWindow::restoreData);
    return page;
}

void MainWindow::backupData()
{
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Backup failed",
            "The database is not available. No backup was created.");
        return;
    }

    const QString defaultPath =
        QDir(QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation))
            .filePath("pfbm-backup.json");

    const QString path = QFileDialog::getSaveFileName(
        this, "Back up PFBM data", defaultPath, "JSON backup (*.json)");

    if (path.isEmpty())
        return;

    std::string error;
    const auto accounts = m_accountRepository.getAll(error);
    if (!error.empty()) {
        QMessageBox::warning(
            this, "Backup failed", QString::fromStdString(error));
        return;
    }

    error.clear();
    const auto transactions = m_transactionRepository.getAll(error);
    if (!error.empty()) {
        QMessageBox::warning(
            this, "Backup failed", QString::fromStdString(error));
        return;
    }

    error.clear();
    const auto budgets = m_budgetRepository.getAll(error);
    if (!error.empty()) {
        QMessageBox::warning(
            this, "Backup failed", QString::fromStdString(error));
        return;
    }

    QJsonArray accountArray;
    QJsonArray transactionArray;
    QJsonArray budgetArray;

    for (const auto& account : accounts) {
        QJsonObject item;
        item["id"] = account.getId();
        item["name"] = QString::fromStdString(account.getName());
        item["type"] = QString::fromStdString(account.getType());
        item["initialBalancePaise"] =
            QString::number(account.getInitialBalancePaise());
        accountArray.append(item);
    }

    for (const auto& transaction : transactions) {
        QJsonObject item;
        item["id"] = transaction.getId();
        item["accountId"] = transaction.getAccountId();
        item["amountPaise"] = QString::number(transaction.getAmountPaise());
        item["type"] =
            transaction.getType() == TransactionType::Income
                ? "income" : "expense";
        item["category"] = QString::fromStdString(transaction.getCategory());
        item["description"] =
            QString::fromStdString(transaction.getDescription());
        item["date"] = QString::fromStdString(transaction.getDate());
        transactionArray.append(item);
    }

    for (const auto& budget : budgets) {
        QJsonObject item;
        item["id"] = budget.getId();
        item["accountId"] = budget.getAccountId();
        item["category"] = QString::fromStdString(budget.getCategory());
        item["month"] = budget.getMonth();
        item["year"] = budget.getYear();
        item["limitPaise"] = QString::number(budget.getLimitPaise());
        budgetArray.append(item);
    }

    QJsonObject backup;
    backup["format"] = "PFBM-backup-v3";
    backup["accounts"] = accountArray;
    backup["transactions"] = transactionArray;
    backup["budgets"] = budgetArray;

    const QByteArray payload =
        QJsonDocument(backup).toJson(QJsonDocument::Indented);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Backup failed", file.errorString());
        return;
    }

    if (file.write(payload) != payload.size()) {
        const QString message = file.errorString();
        file.cancelWriting();
        QMessageBox::warning(this, "Backup failed", message);
        return;
    }

    if (!file.commit()) {
        QMessageBox::warning(this, "Backup failed", file.errorString());
        return;
    }

    QMessageBox::information(
        this, "Backup complete",
        QString("Database backup saved successfully.\n%1").arg(path));
}

void MainWindow::restoreData()
{
    if (!m_databaseReady) {
        QMessageBox::warning(
            this, "Restore failed", "The database is not available.");
        return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, "Restore PFBM backup",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "JSON backup (*.json)");

    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Restore failed", file.errorString());
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        QMessageBox::warning(
            this, "Restore failed", "The selected file is not valid JSON.");
        return;
    }

    const QJsonObject root = document.object();

    const QString backupFormat = root.value("format").toString();
    const bool isV2Backup = backupFormat == "PFBM-backup-v2";
    const bool isV3Backup = backupFormat == "PFBM-backup-v3";
    if ((!isV2Backup && !isV3Backup) ||
        !root.value("accounts").isArray() ||
        !root.value("transactions").isArray() ||
        !root.value("budgets").isArray()) {
        QMessageBox::warning(
            this, "Restore failed",
            "This is not a supported PFBM v2 or v3 backup.");
        return;
    }

    // Strict field readers: reject missing fields and invalid JSON types.
    auto readInt = [](
        const QJsonObject& object,
        const QString& key,
        int& result,
        int minimum,
        int maximum) -> bool
    {
        const QJsonValue value = object.value(key);
        if (!value.isDouble())
            return false;

        const double number = value.toDouble();
        if (!std::isfinite(number) ||
            std::floor(number) != number ||
            number < minimum || number > maximum)
            return false;

        result = static_cast<int>(number);
        return true;
    };

    auto readString = [](
        const QJsonObject& object,
        const QString& key,
        QString& result,
        bool allowEmpty) -> bool
    {
        const QJsonValue value = object.value(key);
        if (!value.isString())
            return false;

        result = value.toString();
        return allowEmpty || !result.trimmed().isEmpty();
    };

    auto readPaise = [](
        const QJsonObject& object,
        const QString& key,
        std::int64_t& result,
        bool mustBePositive) -> bool
    {
        const QJsonValue value = object.value(key);
        if (!value.isString())
            return false;

        bool ok = false;
        const qlonglong parsed = value.toString().toLongLong(&ok);
        if (!ok || (mustBePositive && parsed <= 0))
            return false;

        result = static_cast<std::int64_t>(parsed);
        return true;
    };

    std::vector<Account> accounts;
    std::vector<Transaction> transactions;
    std::vector<Budget> budgets;

    try {
        const QJsonArray accountArray = root.value("accounts").toArray();
        const QJsonArray transactionArray = root.value("transactions").toArray();
        const QJsonArray budgetArray = root.value("budgets").toArray();

        for (const QJsonValue& value : accountArray) {
            if (!value.isObject())
                throw std::runtime_error("An account entry is not an object.");

            const QJsonObject item = value.toObject();
            int id = 0;
            QString name, type;
            std::int64_t balance = 0;

            if (!readInt(item, "id", id, 1,
                         std::numeric_limits<int>::max()) ||
                !readString(item, "name", name, false) ||
                !readString(item, "type", type, false) ||
                !readPaise(item, "initialBalancePaise", balance, false)) {
                throw std::runtime_error(
                    "An account entry contains missing or invalid fields.");
            }

            accounts.emplace_back(
                id, name.toStdString(), type.toStdString(), balance);
        }

        for (const QJsonValue& value : transactionArray) {
            if (!value.isObject())
                throw std::runtime_error(
                    "A transaction entry is not an object.");

            const QJsonObject item = value.toObject();
            int id = 0, accountId = 0;
            std::int64_t amount = 0;
            QString type, category, description, date;

            if (!readInt(item, "id", id, 1,
                         std::numeric_limits<int>::max()) ||
                !readInt(item, "accountId", accountId, 1,
                         std::numeric_limits<int>::max()) ||
                !readPaise(item, "amountPaise", amount, true) ||
                !readString(item, "type", type, false) ||
                !readString(item, "category", category, false) ||
                !readString(item, "description", description, false) ||
                !readString(item, "date", date, false)) {
                throw std::runtime_error(
                    "A transaction entry contains missing or invalid fields.");
            }

            const QDate parsedDate = QDate::fromString(date, Qt::ISODate);
            if (!parsedDate.isValid() ||
                parsedDate.toString(Qt::ISODate) != date) {
                throw std::runtime_error(
                    "A transaction contains an invalid ISO date.");
            }

            TransactionType transactionType;
            if (type == "income") {
                transactionType = TransactionType::Income;
            } else if (type == "expense") {
                transactionType = TransactionType::Expense;
            } else {
                throw std::runtime_error(
                    "A transaction has an unsupported type.");
            }

            transactions.emplace_back(
                id, accountId, amount, transactionType,
                category.toStdString(), description.toStdString(),
                date.toStdString());
        }

        for (const QJsonValue& value : budgetArray) {
            if (!value.isObject())
                throw std::runtime_error("A budget entry is not an object.");

            const QJsonObject item = value.toObject();
            int id = 0, accountId = 0, month = 0, year = 0;
            QString category;
            std::int64_t limit = 0;

            const bool commonFieldsValid =
                readInt(item, "id", id, 1,
                        std::numeric_limits<int>::max()) &&
                readString(item, "category", category, false) &&
                readInt(item, "month", month, 1, 12) &&
                readInt(item, "year", year, 1, 9999) &&
                readPaise(item, "limitPaise", limit, true);

            bool accountFieldValid = true;
            if (isV3Backup) {
                accountFieldValid = readInt(
                    item, "accountId", accountId, 0,
                    std::numeric_limits<int>::max());
            } else if (item.contains("accountId")) {
                // Be tolerant of v2 backup files that may have been extended
                // manually, while still validating the optional field.
                accountFieldValid = readInt(
                    item, "accountId", accountId, 0,
                    std::numeric_limits<int>::max());
            }

            if (!commonFieldsValid || !accountFieldValid) {
                throw std::runtime_error(
                    "A budget entry contains missing or invalid fields.");
            }

            // Version 2 backups have no accountId, so those budgets are
            // restored as unassigned and can be assigned using Edit Budget.
            budgets.emplace_back(
                id, accountId, category.toStdString(), month, year, limit);
        }
    } catch (const std::exception& exception) {
        QMessageBox::warning(
            this, "Restore failed",
            QString("The backup could not be validated:\n%1")
                .arg(QString::fromUtf8(exception.what())));
        return;
    }

    const auto choice = QMessageBox::question(
        this,
        "Confirm restore",
        "This will replace all accounts, transactions, and budgets "
        "in the SQLite database. Continue only if you trust this backup "
        "and have saved any data you need to keep.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (choice != QMessageBox::Yes)
        return;

    std::string restoreError;
    if (!m_backupRestoreService.replaceAll(
            accounts, transactions, budgets, restoreError)) {
        QMessageBox::warning(
            this, "Restore failed",
            QString("The database was not restored:\n%1")
                .arg(QString::fromStdString(restoreError)));
        return;
    }

    // Refresh all views from the restored database.
    refreshAccountsTable();
    refreshAccountScopedViews();

    QMessageBox::information(
        this, "Restore complete",
        "The backup was restored to SQLite successfully.");
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

    // Refresh persisted data before displaying data-dependent pages.
    if (pageIndex == 0) {
        refreshTransactionsTable();
        refreshBudgetsTable();
        refreshDashboard();
    } else if (pageIndex == 1) {
        refreshTransactionsTable();
    } else if (pageIndex == 2) {
        refreshAccountsTable();
    } else if (pageIndex == 3) {
        refreshTransactionsTable();
        refreshBudgetsTable();
    } else if (pageIndex == 4) {
        refreshTransactionsTable();
        refreshBudgetsTable();
        if (m_reportWasGenerated)
            runReport();
    } else if (pageIndex == 5) {
        refreshAccountsTable();
    }

    m_pages->setCurrentIndex(pageIndex);

    if (m_pageTitle)
        m_pageTitle->setText(navigationItems.value(pageIndex, "Dashboard"));

    if (m_statusLabel) {
        if (!m_databaseReady) {
            m_statusLabel->setText(
                QString("Database unavailable: %1").arg(m_databaseError));
        } else if (pageIndex == 0) {
            m_statusLabel->setText(
                "SQLite connected • Dashboard refreshed from saved data");
        } else {
            m_statusLabel->setText(
                "SQLite connected • "
                + navigationItems.value(pageIndex) + " ready");
        }
    }

    if (m_navigationGroup && m_navigationGroup->button(pageIndex))
        m_navigationGroup->button(pageIndex)->setChecked(true);
}

void MainWindow::styleComboBoxPopup(QComboBox *combo) const
{
    if (!combo)
        return;

    const bool dark = m_themePreference == "dark";
    const QColor background = dark ? QColor("#101c2e") : QColor("#ffffff");
    const QColor text = dark ? QColor("#e6edf7") : QColor("#172033");
    const QColor border = dark ? QColor("#34465f") : QColor("#d9e1ec");
    const QColor selectedBackground = dark ? QColor("#315fca") : QColor("#dbeafe");
    const QColor hoverBackground = dark ? QColor("#203451") : QColor("#edf3ff");
    const QColor selectedText = dark ? QColor("#ffffff") : QColor("#172033");

    auto *view = combo->view();
    QPalette palette = view->palette();
    for (const QPalette::ColorGroup group : {
             QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        palette.setColor(group, QPalette::Base, background);
        palette.setColor(group, QPalette::Window, background);
        palette.setColor(group, QPalette::Text, text);
        palette.setColor(group, QPalette::WindowText, text);
        palette.setColor(group, QPalette::Highlight, selectedBackground);
        palette.setColor(group, QPalette::HighlightedText, selectedText);
    }
    view->setPalette(palette);
    view->setAutoFillBackground(true);
    view->setStyleSheet(QString::fromUtf8(R"(
        QAbstractItemView {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            outline: 0;
        }
        QAbstractItemView::item {
            color: %2;
            min-height: 24px;
            padding: 6px 10px;
        }
        QAbstractItemView::item:hover {
            color: %6;
            background-color: %5;
        }
        QAbstractItemView::item:selected {
            color: %6;
            background-color: %4;
        }
    )").arg(background.name(), text.name(), border.name(),
            selectedBackground.name(), hoverBackground.name(), selectedText.name()));

    QWidget *popup = view->window();
    if (popup && popup != combo) {
        popup->setPalette(palette);
        popup->setAutoFillBackground(true);
    }
}

void MainWindow::applyStyles()
{
    QString styles = QString::fromUtf8(R"(
        QMainWindow, QWidget#centralSurface {
            background: #f6f8fb;
            color: #172033;
            font-family: "Segoe UI", "Arial";
            font-size: 10pt;
        }
        QDialog {
            background: #f6f8fb;
            color: #172033;
        }
        QWidget#sidebar {
            background: #111c2f;
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
            color: #c5d0df;
            background: transparent;
            border: none;
            border-radius: 10px;
            padding: 12px 14px;
            font-size: 10pt;
        }
        QPushButton#navigationButton:hover {
            background: #1c2c45;
            color: #ffffff;
        }
        QPushButton#navigationButton:checked {
            color: #ffffff;
            background: #365fce;
            font-weight: 700;
        }
        QPushButton {
            color: #263750;
            background: #ffffff;
            border: 1px solid #d9e1ec;
            border-radius: 10px;
            padding: 9px 15px;
            min-height: 20px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #f0f5ff;
            border-color: #a9bfeb;
        }
        QPushButton:pressed {
            background: #e4edff;
        }
        QPushButton:focus {
            border: 2px solid #4f7fe0;
        }
        QFrame#sidebarNote {
            background: #1a2a42;
            border: 1px solid #2c405b;
            border-radius: 12px;
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
            font-size: 25pt;
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
        QComboBox#activeAccountCombo {
            background: #ffffff;
            color: #172033;
            border: 1px solid #d8e2f0;
            border-radius: 8px;
            padding: 7px 10px;
            min-height: 18px;
            font-weight: 600;
        }
        QComboBox#activeAccountCombo:hover {
            border-color: #365fce;
        }
        QComboBox, QLineEdit, QDateEdit, QDoubleSpinBox {
            background: #ffffff;
            color: #172033;
            border: 1px solid #d9e1ec;
            border-radius: 8px;
            padding: 8px 10px;
            min-height: 18px;
            selection-background-color: #dbeafe;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 28px;
            border: none;
            border-left: 1px solid #d9e1ec;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: url(:/icons/chevron-down-light.xpm);
            width: 9px;
            height: 6px;
        }
        QComboBox:focus, QLineEdit:focus, QDateEdit:focus, QDoubleSpinBox:focus {
            border: 1px solid #4f7fe0;
        }
        QFrame#welcomePanel {
            background: #edf3ff;
            border: 1px solid #dbe7ff;
            border-radius: 16px;
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
            border: 1px solid #e6ebf2;
            border-radius: 14px;
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
            border-radius: 10px;
            padding: 10px 13px;
            font-weight: 700;
        }
        QPushButton#primaryActionButton:hover { background: #2b50b5; }
        QPushButton#secondaryActionButton {
            color: #263750;
            background: #ffffff;
            border: 1px solid #dce3ed;
            border-radius: 10px;
            padding: 10px 13px;
        }
        QPushButton#secondaryActionButton:hover { background: #edf2fa; }
        QPushButton#dangerActionButton {
            color: #a12a35;
            background: #fff1f1;
            border: 1px solid #f3d2d5;
            border-radius: 10px;
            padding: 10px 13px;
        }
        QPushButton#dangerActionButton:hover { background: #ffe4e6; }
        QTableWidget#accountsTable {
            background: #ffffff;
            alternate-background-color: #f7f9fc;
            border: 1px solid #e6ebf2;
            border-radius: 12px;
            gridline-color: #edf1f6;
            selection-background-color: #e7edff;
            selection-color: #172033;
        }
        QTableWidget#accountsTable QHeaderView::section {
            background: #f7f9fc;
            color: #536176;
            border: none;
            border-bottom: 1px solid #e6ebf2;
            padding: 11px 9px;
            font-weight: 700;
        }
        QProgressBar {
            background: #edf1f6;
            color: #526076;
            border: none;
            border-radius: 6px;
            min-height: 18px;
            max-height: 20px;
            text-align: center;
        }
        QProgressBar::chunk {
            background: #3974df;
            border-radius: 6px;
        }
        QLabel#fieldHint { color: #778398; font-size: 9pt; }
        QFrame#emptyStatePanel { background: #ffffff; border: 1px dashed #cbd5e1; border-radius: 12px; }
        QLabel#emptyStateTitle { color: #172033; font-size: 15pt; font-weight: 700; }
        QTableWidget#accountsTable::item { background: transparent; color: #263750; padding: 8px; border: none; }
        QTableWidget#accountsTable::item:selected { background: #dce8ff; color: #172033; }
        QTableWidget#accountsTable::item:focus { outline: none; }
        QTableWidget#accountsTable QTableCornerButton::section { background: #f7f9fc; border: none; }
        QPushButton:disabled { color: #9aa4b2; background: #f0f2f5; border-color: #e2e6ec; }
        QLabel#statusLabel {
            color: #7b8799;
            font-size: 8pt;
            padding-top: 5px;
        }
    )");

    if (m_themePreference == "dark") {
        styles += QString::fromUtf8(R"(
            QMainWindow, QWidget#centralSurface {
                background: #0c1422;
                color: #e6edf7;
            }
            QDialog { background: #0c1422; color: #e6edf7; }
            QWidget#sidebar { background: #080f1b; }
            QFrame#contentPanel, QFrame#metricCard, QFrame#emptyStatePanel {
                background: #121e30;
                border-color: #26374d;
            }
            QFrame#welcomePanel {
                background: #142442;
                border-color: #263d63;
            }
            QLabel#pageTitle, QLabel#metricValue, QLabel#panelTitle,
            QLabel#placeholderTitle, QLabel#emptyStateTitle,
            QLabel#budgetRowTitle { color: #edf3fc; }
            QLabel#metricCaption { color: #91a2b9; }
            QLabel#pageSubtitle, QLabel#panelHint, QLabel#metricDetail,
            QLabel#fieldHint, QLabel#budgetRowValue, QLabel#statusLabel,
            QLabel#placeholderDescription { color: #a4b2c5; }
            QLabel#welcomeTitle { color: #d7e5ff; }
            QLabel#welcomeText { color: #b6c8e5; }
            QLabel#periodTag, QLabel#placeholderTag {
                color: #c5d8ff;
                background: #1d3153;
            }
            QComboBox, QLineEdit, QDateEdit, QDoubleSpinBox {
                color: #e6edf7;
                background: #0e1929;
                border-color: #34465f;
                selection-background-color: #315fca;
            }
            QComboBox:focus {
                border-color: #4c88f0;
            }
            QComboBox::drop-down {
                border-left-color: #34465f;
            }
            QComboBox::down-arrow {
                image: url(:/icons/chevron-down-dark.xpm);
            }
            QComboBox#activeAccountCombo {
                color: #e6edf7;
                background: #101c2e;
                border-color: #34465f;
            }
            QPushButton {
                color: #dce6f4;
                background: #17253a;
                border-color: #33465f;
            }
            QPushButton:hover {
                color: #ffffff;
                background: #203451;
                border-color: #5478b2;
            }
            QPushButton:pressed { background: #29436a; }
            QPushButton#primaryActionButton {
                color: #ffffff;
                background: #3974df;
                border-color: #3974df;
            }
            QPushButton#primaryActionButton:hover { background: #4a86f4; }
            QPushButton#secondaryActionButton {
                color: #dce6f4;
                background: #17253a;
                border-color: #33465f;
            }
            QPushButton#dangerActionButton {
                color: #ffc2c2;
                background: #3a2028;
                border-color: #69404b;
            }
            QTableWidget#accountsTable {
                color: #dce6f4;
                background: #0f1a2a;
                alternate-background-color: #132033;
                border-color: #26374d;
                gridline-color: #26374d;
                selection-background-color: #263e65;
                selection-color: #ffffff;
            }
            QTableWidget#accountsTable QHeaderView::section {
                color: #c4d0e0;
                background: #17253a;
                border-bottom-color: #2a3c53;
            }
            QTableWidget#accountsTable::item { color: #dce6f4; }
            QTableWidget#accountsTable::item:selected {
                color: #ffffff;
                background: #29446e;
            }
            QTableWidget#accountsTable QTableCornerButton::section {
                background: #17253a;
            }
            QScrollArea#settingsScroll {
                background: transparent;
                border: none;
            }
            QScrollArea#settingsScroll QWidget#qt_scrollarea_viewport {
                background: transparent;
            }
            QProgressBar {
                color: #dce6f4;
                background: #26364b;
            }
            QProgressBar::chunk { background: #4c88f0; }
            QFrame#sidebarNote {
                background: #111e30;
                border-color: #2b3d55;
            }
            QLabel#sidebarNoteBody { color: #b4c0d1; }
            QPushButton:disabled {
                color: #718096;
                background: #1b2738;
                border-color: #2b3749;
            }
        )");
    }

    setStyleSheet(styles);
    for (auto *combo : findChildren<QComboBox *>())
        styleComboBoxPopup(combo);
}
