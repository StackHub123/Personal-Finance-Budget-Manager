#include "authwindow.h"

#include "services/AuthService.h"

#include <QApplication>
#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPalette>

namespace {
QLabel *makeAuthLabel(const QString &text, const QString &objectName = {})
{
    auto *label = new QLabel(text);
    if (!objectName.isEmpty())
        label->setObjectName(objectName);
    return label;
}
}

AuthWindow::AuthWindow(AuthService &authService, QWidget *parent)
    : QMainWindow(parent), m_authService(authService)
{
    setWindowTitle("Personal Finance & Budgeting Manager");
    setMinimumSize(420, 560);
    resize(500, 680);

    auto *surface = new QWidget(this);
    surface->setObjectName("authSurface");
    auto *layout = new QVBoxLayout(surface);
    layout->setContentsMargins(34, 30, 34, 28);
    layout->setSpacing(16);

    auto *brand = new QFrame;
    brand->setObjectName("authBrand");
    auto *brandLayout = new QVBoxLayout(brand);
    brandLayout->setContentsMargins(0, 4, 0, 4);
    brandLayout->setSpacing(4);
    brandLayout->addWidget(makeAuthLabel("PFBM", "authBrandMark"));
    brandLayout->addWidget(makeAuthLabel(
        "PERSONAL FINANCE & BUDGETING MANAGER", "authBrandSubtitle"));
    layout->addWidget(brand);

    m_progress = new QProgressBar;
    m_progress->setObjectName("authProgress");
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(4);
    m_progress->hide();
    layout->addWidget(m_progress);

    m_stack = new QStackedWidget;
    m_stack->setObjectName("authStack");
    m_stack->addWidget(buildLoginPage());
    m_stack->addWidget(buildSignupPage());
    layout->addWidget(m_stack, 1);

    auto *local = makeAuthLabel(
        "Your financial records are stored locally on this device.",
        "authFooter");
    local->setWordWrap(true);
    local->setAlignment(Qt::AlignCenter);
    layout->addWidget(local);
    setCentralWidget(surface);

    connect(m_showLoginPassword, &QCheckBox::toggled, this, [this](bool show) {
        m_loginPassword->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    });
    connect(m_showSignupPassword, &QCheckBox::toggled, this, [this](bool show) {
        const auto mode = show ? QLineEdit::Normal : QLineEdit::Password;
        m_signupPassword->setEchoMode(mode);
        m_signupConfirmation->setEchoMode(mode);
    });
    refreshTheme();
}

QWidget *AuthWindow::buildLoginPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);
    layout->addWidget(makeAuthLabel("Welcome back", "authTitle"));
    auto *description = makeAuthLabel("Sign in to continue to your finances.", "authHint");
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *card = new QFrame;
    card->setObjectName("authCard");
    auto *formLayout = new QVBoxLayout(card);
    formLayout->setContentsMargins(22, 22, 22, 22);
    formLayout->setSpacing(14);

    auto *form = new QFormLayout;
    form->setSpacing(12);
    m_loginEmail = new QLineEdit;
    m_loginEmail->setObjectName("loginEmail");
    m_loginEmail->setPlaceholderText("you@example.com");
    m_loginEmail->setClearButtonEnabled(true);
    m_loginEmail->setMaxLength(254);
    m_loginPassword = new QLineEdit;
    m_loginPassword->setObjectName("loginPassword");
    m_loginPassword->setEchoMode(QLineEdit::Password);
    m_loginPassword->setPlaceholderText("Enter your password");
    form->addRow("Email address", m_loginEmail);
    form->addRow("Password", m_loginPassword);
    formLayout->addLayout(form);

    m_showLoginPassword = new QCheckBox("Show password");
    m_showLoginPassword->setObjectName("showLoginPassword");
    formLayout->addWidget(m_showLoginPassword);
    m_loginMessage = makeAuthLabel({}, "loginMessage");
    m_loginMessage->setWordWrap(true);
    formLayout->addWidget(m_loginMessage);

    auto *loginButton = new QPushButton("Log In");
    loginButton->setObjectName("loginButton");
    loginButton->setDefault(true);
    formLayout->addWidget(loginButton);
    layout->addWidget(card);

    auto *signupRow = new QHBoxLayout;
    signupRow->addStretch();
    signupRow->addWidget(makeAuthLabel("Don't have an account?"));
    auto *signupLink = new QPushButton("Sign Up");
    signupLink->setObjectName("gotoSignupButton");
    signupLink->setFlat(true);
    signupRow->addWidget(signupLink);
    signupRow->addStretch();
    layout->addLayout(signupRow);
    layout->addStretch();

    connect(loginButton, &QPushButton::clicked, this, &AuthWindow::login);
    connect(signupLink, &QPushButton::clicked, this, &AuthWindow::showSignup);
    connect(m_loginPassword, &QLineEdit::returnPressed, loginButton, &QPushButton::click);
    return page;
}

QWidget *AuthWindow::buildSignupPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);
    layout->addWidget(makeAuthLabel("Create your account", "authTitle"));
    auto *description = makeAuthLabel(
        "Register to keep your financial records in a separate local profile.",
        "authHint");
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *card = new QFrame;
    card->setObjectName("authCard");
    auto *formLayout = new QVBoxLayout(card);
    formLayout->setContentsMargins(22, 22, 22, 22);
    formLayout->setSpacing(12);
    auto *form = new QFormLayout;
    form->setSpacing(10);
    m_signupEmail = new QLineEdit;
    m_signupEmail->setObjectName("signupEmail");
    m_signupEmail->setPlaceholderText("you@example.com");
    m_signupEmail->setClearButtonEnabled(true);
    m_signupEmail->setMaxLength(254);
    m_signupPassword = new QLineEdit;
    m_signupPassword->setObjectName("signupPassword");
    m_signupPassword->setEchoMode(QLineEdit::Password);
    m_signupPassword->setPlaceholderText("At least 8 characters");
    m_signupConfirmation = new QLineEdit;
    m_signupConfirmation->setObjectName("signupConfirmation");
    m_signupConfirmation->setEchoMode(QLineEdit::Password);
    m_signupConfirmation->setPlaceholderText("Enter the password again");
    form->addRow("Email address", m_signupEmail);
    form->addRow("Password", m_signupPassword);
    form->addRow("Confirm password", m_signupConfirmation);
    formLayout->addLayout(form);

    auto *passwordHint = makeAuthLabel(
        "Use at least 8 characters. Longer, unique passwords are safer.",
        "passwordPolicyHint");
    passwordHint->setWordWrap(true);
    formLayout->addWidget(passwordHint);
    m_showSignupPassword = new QCheckBox("Show password");
    m_showSignupPassword->setObjectName("showSignupPassword");
    formLayout->addWidget(m_showSignupPassword);
    m_signupMessage = makeAuthLabel({}, "signupMessage");
    m_signupMessage->setWordWrap(true);
    formLayout->addWidget(m_signupMessage);
    auto *createButton = new QPushButton("Create Account");
    createButton->setObjectName("signupButton");
    createButton->setDefault(true);
    formLayout->addWidget(createButton);
    layout->addWidget(card);

    auto *loginRow = new QHBoxLayout;
    loginRow->addStretch();
    loginRow->addWidget(makeAuthLabel("Already have an account?"));
    auto *loginLink = new QPushButton("Log In");
    loginLink->setObjectName("gotoLoginButton");
    loginLink->setFlat(true);
    loginRow->addWidget(loginLink);
    loginRow->addStretch();
    layout->addLayout(loginRow);
    layout->addStretch();

    connect(createButton, &QPushButton::clicked, this, &AuthWindow::registerUser);
    connect(loginLink, &QPushButton::clicked, this, &AuthWindow::showLogin);
    connect(m_signupConfirmation, &QLineEdit::returnPressed,
            createButton, &QPushButton::click);
    return page;
}

void AuthWindow::showLogin()
{
    if (m_processing)
        return;
    m_stack->setCurrentIndex(0);
    m_loginEmail->setFocus();
}

void AuthWindow::showSignup()
{
    if (m_processing)
        return;
    m_stack->setCurrentIndex(1);
    m_signupEmail->setFocus();
}

void AuthWindow::setProcessing(bool processing, const QString &message)
{
    m_processing = processing;
    if (m_progress)
        m_progress->setVisible(processing);
    if (m_loginMessage && m_stack->currentIndex() == 0)
        m_loginMessage->setText(message);
    if (m_signupMessage && m_stack->currentIndex() == 1)
        m_signupMessage->setText(message);
    if (m_loginEmail) m_loginEmail->setEnabled(!processing);
    if (m_loginPassword) m_loginPassword->setEnabled(!processing);
    if (m_signupEmail) m_signupEmail->setEnabled(!processing);
    if (m_signupPassword) m_signupPassword->setEnabled(!processing);
    if (m_signupConfirmation) m_signupConfirmation->setEnabled(!processing);
    if (m_loginMessage) m_loginMessage->setProperty("busy", processing);
    QApplication::processEvents();
}

void AuthWindow::login()
{
    if (m_processing)
        return;
    setProcessing(true, QStringLiteral("Signing in…"));
    QString error;
    const bool success = m_authService.authenticate(
        m_loginEmail->text(), m_loginPassword->text(), error);
    m_loginPassword->clear();
    setProcessing(false, success ? QString() : error);
    if (success)
        emit authenticated();
}

void AuthWindow::registerUser()
{
    if (m_processing)
        return;
    setProcessing(true, QStringLiteral("Creating your local profile…"));
    QString error;
    const bool success = m_authService.registerUser(
        m_signupEmail->text(), m_signupPassword->text(),
        m_signupConfirmation->text(), error);
    m_signupPassword->clear();
    m_signupConfirmation->clear();
    setProcessing(false, success ? QString() : error);
    if (success)
        emit authenticated();
}

void AuthWindow::resetToLogin()
{
    if (m_loginPassword) m_loginPassword->clear();
    if (m_signupPassword) m_signupPassword->clear();
    if (m_signupConfirmation) m_signupConfirmation->clear();
    if (m_loginMessage) m_loginMessage->clear();
    if (m_signupMessage) m_signupMessage->clear();
    if (m_showLoginPassword) m_showLoginPassword->setChecked(false);
    if (m_showSignupPassword) m_showSignupPassword->setChecked(false);
    showLogin();
}

void AuthWindow::refreshTheme()
{
    applyTheme();
}

void AuthWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    applyTheme();
}

void AuthWindow::applyTheme()
{
    QSettings settings;
    const bool dark = settings.value("settings/theme", "light").toString() == "dark";
    const QString background = dark ? "#0c1422" : "#f6f8fb";
    const QString card = dark ? "#121e30" : "#ffffff";
    const QString field = dark ? "#0e1929" : "#ffffff";
    const QString text = dark ? "#e6edf7" : "#172033";
    const QString muted = dark ? "#a4b2c5" : "#667085";
    const QString border = dark ? "#34465f" : "#d9e1ec";
    const QString accent = dark ? "#3974df" : "#365fce";
    setStyleSheet(QString::fromUtf8(R"(
        QMainWindow, QWidget#authSurface { background: %1; color: %4; }
        QFrame#authCard { background: %2; border: 1px solid %6; border-radius: 16px; }
        QLabel#authBrandMark { color: %4; font-size: 26pt; font-weight: 800; letter-spacing: 1px; }
        QLabel#authBrandSubtitle { color: %5; font-size: 8pt; font-weight: 700; letter-spacing: 1px; }
        QLabel#authTitle { color: %4; font-size: 22pt; font-weight: 750; }
        QLabel#authHint, QLabel#authFooter, QLabel#passwordPolicyHint { color: %5; }
        QLabel#loginMessage, QLabel#signupMessage { color: %7; font-weight: 600; }
        QLineEdit {
            background: %3; color: %4; border: 1px solid %6;
            border-radius: 8px; padding: 10px 11px; min-height: 20px;
            selection-background-color: %8;
        }
        QLineEdit:focus { border: 2px solid %8; }
        QCheckBox { color: %5; spacing: 7px; }
        QCheckBox::indicator { width: 16px; height: 16px; }
        QPushButton {
            color: white; background: %8; border: none; border-radius: 9px;
            padding: 11px 16px; min-height: 20px; font-weight: 700;
        }
        QPushButton:hover { background: %9; }
        QPushButton:disabled { color: %5; background: %6; }
        QPushButton#gotoSignupButton, QPushButton#gotoLoginButton {
            color: %8; background: transparent; padding: 4px 6px; min-height: 16px;
        }
        QPushButton#gotoSignupButton:hover, QPushButton#gotoLoginButton:hover {
            text-decoration: underline; background: transparent;
        }
        QProgressBar { background: %6; border: none; border-radius: 2px; }
        QProgressBar::chunk { background: %8; border-radius: 2px; }
    )").arg(background, card, field, text, muted, border,
            dark ? QStringLiteral("#ffb4b4") : QStringLiteral("#b42318"),
            accent, dark ? QStringLiteral("#4a86f4") : QStringLiteral("#2b50b5")));

    QPalette inputPalette = palette();
    inputPalette.setColor(QPalette::PlaceholderText,
                          dark ? QColor("#8192a9") : QColor("#8792a2"));
    for (auto *input : findChildren<QLineEdit *>())
        input->setPalette(inputPalette);
}
