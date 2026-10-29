#ifndef AUTHWINDOW_H
#define AUTHWINDOW_H

#include <QMainWindow>

class AuthService;
class QCheckBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QStackedWidget;

class AuthWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit AuthWindow(AuthService &authService, QWidget *parent = nullptr);
    void resetToLogin();
    void refreshTheme();

signals:
    void authenticated();

protected:
    void showEvent(QShowEvent *event) override;

private:
    QWidget *buildLoginPage();
    QWidget *buildSignupPage();
    void showLogin();
    void showSignup();
    void login();
    void registerUser();
    void setProcessing(bool processing, const QString &message);
    void applyTheme();

    AuthService &m_authService;
    QStackedWidget *m_stack = nullptr;
    QLineEdit *m_loginEmail = nullptr;
    QLineEdit *m_loginPassword = nullptr;
    QCheckBox *m_showLoginPassword = nullptr;
    QLabel *m_loginMessage = nullptr;
    QLineEdit *m_signupEmail = nullptr;
    QLineEdit *m_signupPassword = nullptr;
    QLineEdit *m_signupConfirmation = nullptr;
    QCheckBox *m_showSignupPassword = nullptr;
    QLabel *m_signupMessage = nullptr;
    QProgressBar *m_progress = nullptr;
    bool m_processing = false;
};

#endif // AUTHWINDOW_H
