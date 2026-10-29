#include "authwindow.h"

#include "database/DatabaseManager.h"
#include "models/Account.h"
#include "repositories/AccountRepository.h"
#include "repositories/BudgetRepository.h"
#include "repositories/TransactionRepository.h"
#include "repositories/UserRepository.h"
#include "services/AppDataPaths.h"
#include "services/AuthService.h"
#include "services/LegacyDataMigrationService.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

QString readPasswordHash(DatabaseManager &database, const QString &email)
{
    sqlite3_stmt *statement = nullptr;
    const char *sql = "SELECT password_hash FROM users WHERE email = ?;";
    require(sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr) == SQLITE_OK,
            "prepare password hash lookup");
    const QByteArray emailUtf8 = email.toUtf8();
    sqlite3_bind_text(statement, 1, emailUtf8.constData(), emailUtf8.size(), SQLITE_TRANSIENT);
    const int result = sqlite3_step(statement);
    require(result == SQLITE_ROW, "registered user has a stored password hash");
    const auto *value = sqlite3_column_text(statement, 0);
    const QString hash = value
        ? QString::fromUtf8(reinterpret_cast<const char *>(value))
        : QString();
    sqlite3_finalize(statement);
    return hash;
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir temporaryDirectory;
    require(temporaryDirectory.isValid(), "temporary data directory is available");

    const QString authPath = QDir(temporaryDirectory.path()).filePath("auth.sqlite");
    DatabaseManager authDatabase;
    std::string databaseError;
    require(authDatabase.open(authPath.toStdString(), databaseError),
            "open temporary authentication database");
    UserRepository users(authDatabase);
    QString error;
    require(users.initializeSchema(error), "create authentication schema");
    AuthService auth(users);

    require(!auth.registerUser("", "ValidPass123", "ValidPass123", error),
            "reject empty registration fields");
    require(!auth.registerUser("bad-email", "ValidPass123", "ValidPass123", error),
            "reject invalid email");
    require(!auth.registerUser("person@example.com", "short", "short", error),
            "reject weak password");
    require(!auth.registerUser("person@example.com", "ValidPass123", "Different123", error),
            "reject password confirmation mismatch");

    AuthWindow window(auth);
    require(window.findChild<QLineEdit *>("loginPassword")->echoMode() == QLineEdit::Password,
            "login password is masked by default");
    int authenticatedSignals = 0;
    QObject::connect(&window, &AuthWindow::authenticated, [&authenticatedSignals]() {
        ++authenticatedSignals;
    });
    window.findChild<QPushButton *>("gotoSignupButton")->click();
    auto *signupEmail = window.findChild<QLineEdit *>("signupEmail");
    auto *signupPassword = window.findChild<QLineEdit *>("signupPassword");
    auto *signupConfirmation = window.findChild<QLineEdit *>("signupConfirmation");
    require(signupPassword->echoMode() == QLineEdit::Password,
            "signup password is masked by default");
    signupEmail->setText("  Person@Example.com ");
    signupPassword->setText("Correct-Horse-42");
    signupConfirmation->setText("Correct-Horse-42");
    window.findChild<QPushButton *>("signupButton")->click();
    require(authenticatedSignals == 1, "successful signup emits the authenticated signal");
    require(auth.currentUser().has_value(), "registration creates an authenticated session");
    require(auth.currentUser()->email() == QStringLiteral("person@example.com"),
            "registration trims and canonicalizes email");

    const QString storedHash = readPasswordHash(authDatabase, "person@example.com");
    require(storedHash.startsWith("scrypt$1$"), "password uses versioned scrypt hash");
    require(storedHash != QStringLiteral("Correct-Horse-42"),
            "database does not store plaintext password");
    require(!auth.registerUser("PERSON@example.com", "AnotherPass-42",
                               "AnotherPass-42", error),
            "duplicate email is rejected without case sensitivity");

    auth.logout();
    require(!auth.hasAuthenticatedUser(), "logout clears the authenticated session");
    require(!auth.authenticate("person@example.com", "Incorrect-Pass-42", error),
            "incorrect password is rejected");
    const QString wrongPasswordError = error;
    require(!auth.authenticate("unknown@example.com", "Incorrect-Pass-42", error),
            "unknown email is rejected");
    require(error == wrongPasswordError && error == QStringLiteral("Invalid email or password."),
            "unknown and incorrect credentials use the same message");
    require(auth.authenticate("  PERSON@EXAMPLE.COM ", "Correct-Horse-42", error),
            "normalized email and correct password authenticate");
    const int authenticatedUserId = auth.currentUser()->id();

    QString databaseAPath;
    QString databaseBPath;
    require(AppDataPaths::ensureFinancialDatabaseDirectory(
                authenticatedUserId, temporaryDirectory.path(), databaseAPath, error),
            "create first profile data path");
    require(AppDataPaths::ensureFinancialDatabaseDirectory(
                authenticatedUserId + 1, temporaryDirectory.path(), databaseBPath, error),
            "create separate second profile data path");
    DatabaseManager financeA;
    DatabaseManager financeB;
    require(financeA.open(databaseAPath.toStdString(), databaseError) &&
                financeA.initializeSchema(databaseError),
            "initialize first user's finance database");
    require(financeB.open(databaseBPath.toStdString(), databaseError) &&
                financeB.initializeSchema(databaseError),
            "initialize second user's finance database");
    AccountRepository accountsA(financeA);
    AccountRepository accountsB(financeB);
    TransactionRepository transactionsA(financeA);
    TransactionRepository transactionsB(financeB);
    BudgetRepository budgetsA(financeA);
    BudgetRepository budgetsB(financeB);
    Account account(0, "Private savings", "Savings", 125000);
    require(accountsA.add(account, databaseError), "save financial account for first user");
    Transaction transaction(0, account.getId(), 2500, TransactionType::Expense,
                            "Food", "Private purchase", "2026-10-29");
    require(transactionsA.add(transaction, databaseError),
            "save transaction for first user");
    Budget budget(0, account.getId(), "Food", 10, 2026, 50000);
    require(budgetsA.add(budget, databaseError), "save budget for first user");
    require(accountsA.getAll(databaseError).size() == 1,
            "first user can read own financial account");
    require(transactionsA.getAll(databaseError).size() == 1,
            "first user can read own transaction");
    require(budgetsA.getAll(databaseError).size() == 1,
            "first user can read own budget");
    require(accountsB.getAll(databaseError).empty(),
            "second user's finance database cannot read first user's account");
    require(transactionsB.getAll(databaseError).empty(),
            "second user's finance database cannot read first user's transaction");
    require(budgetsB.getAll(databaseError).empty(),
            "second user's finance database cannot read first user's budget");

    const QString legacyPath = QDir(temporaryDirectory.path()).filePath("legacy.db");
    const QString migratedPath = QDir(temporaryDirectory.path()).filePath("migrated.db");
    DatabaseManager legacyDatabase;
    DatabaseManager migratedDatabase;
    require(legacyDatabase.open(legacyPath.toStdString(), databaseError) &&
                legacyDatabase.initializeSchema(databaseError),
            "initialize legacy data store");
    AccountRepository legacyAccounts(legacyDatabase);
    Account legacyAccount(0, "Legacy wallet", "Cash", 9100);
    require(legacyAccounts.add(legacyAccount, databaseError),
            "write legacy financial record");
    require(migratedDatabase.open(migratedPath.toStdString(), databaseError) &&
                migratedDatabase.initializeSchema(databaseError),
            "initialize target data store");
    AccountRepository migratedAccounts(migratedDatabase);
    TransactionRepository migratedTransactions(migratedDatabase);
    BudgetRepository migratedBudgets(migratedDatabase);
    require(LegacyDataMigrationService::importInto(
                legacyPath, migratedDatabase, migratedAccounts,
                migratedTransactions, migratedBudgets, error),
            "import legacy data into a profile");
    require(migratedAccounts.getAll(databaseError).size() == 1,
            "legacy record arrives in the selected profile");
    require(legacyAccounts.getAll(databaseError).size() == 1,
            "legacy database remains intact after import");

    auth.logout();
    require(!auth.currentUser(), "session is empty after final logout");
    std::cout << "Authentication integration tests passed.\n";
    return EXIT_SUCCESS;
}
