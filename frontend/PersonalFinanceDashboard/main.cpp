#include "authwindow.h"
#include "mainwindow.h"

#include "database/DatabaseManager.h"
#include "repositories/UserRepository.h"
#include "services/AppDataPaths.h"
#include "services/AuthService.h"
#include "services/LegacyDataMigrationService.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

#include <memory>
#include <string>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("PersonalFinanceDashboard");
    QApplication::setOrganizationName("PFBM");

    const QString dataDirectory = AppDataPaths::dataDirectory();
    if (!QDir().mkpath(dataDirectory)) {
        QMessageBox::critical(nullptr, "PFBM cannot start",
                              "The application data folder could not be created.");
        return 1;
    }

    DatabaseManager authDatabase;
    std::string databaseError;
    if (!authDatabase.open(AppDataPaths::authenticationDatabase().toStdString(),
                           databaseError)) {
        QMessageBox::critical(nullptr, "PFBM cannot start",
                              "Authentication storage could not be opened. "
                              "Your existing financial data has not been changed.");
        return 1;
    }
    UserRepository users(authDatabase);
    QString authError;
    if (!users.initializeSchema(authError)) {
        QMessageBox::critical(nullptr, "PFBM cannot start", authError);
        return 1;
    }
    AuthService authService(users);
    AuthWindow authWindow(authService);

    std::unique_ptr<MainWindow> mainWindow;
    QObject::connect(&authWindow, &AuthWindow::authenticated, &app, [&]() {
        const auto user = authService.currentUser();
        if (!user || !user->isValid()) {
            authService.logout();
            QMessageBox::warning(&authWindow, "Sign in required",
                                 "Please sign in again to open your profile.");
            authWindow.resetToLogin();
            return;
        }

        QString financePath;
        QString pathError;
        if (!AppDataPaths::ensureFinancialDatabaseDirectory(
                user->id(), dataDirectory, financePath, pathError)) {
            authService.logout();
            QMessageBox::critical(&authWindow, "Profile unavailable", pathError);
            authWindow.resetToLogin();
            return;
        }

        // The pre-authentication database belongs to no known user. Resolve its
        // one-time ownership before creating a profile database so Cancel cannot
        // accidentally suppress the choice on the next sign-in.
        const QString legacyPath = AppDataPaths::legacyFinancialDatabase();
        bool hasLegacyRecords = false;
        if (QFileInfo::exists(legacyPath) &&
            !LegacyDataMigrationService::containsFinancialRecords(
                legacyPath, hasLegacyRecords, pathError)) {
            authService.logout();
            QMessageBox::critical(&authWindow, "Existing data needs attention", pathError);
            authWindow.resetToLogin();
            return;
        }

        bool importLegacy = false;
        const QString claimKey = QStringLiteral("legacy_finance_claim");
        const QString claimForUser = QStringLiteral("pending:%1").arg(user->id());
        if (hasLegacyRecords) {
            const auto storedClaim = users.metadataValue(claimKey, pathError);
            if (!pathError.isEmpty()) {
                authService.logout();
                QMessageBox::critical(&authWindow, "Profile data unavailable",
                                      "The existing-data migration status could not be read.");
                authWindow.resetToLogin();
                return;
            }

            bool mayDecide = storedClaim && *storedClaim == claimForUser;
            if (!storedClaim) {
                bool inserted = false;
                if (!users.insertMetadataIfAbsent(claimKey, claimForUser,
                                                  inserted, pathError)) {
                    authService.logout();
                    QMessageBox::critical(&authWindow, "Profile data unavailable",
                                          "The existing-data choice could not be saved.");
                    authWindow.resetToLogin();
                    return;
                }
                mayDecide = inserted;
            }

            if (mayDecide) {
                QMessageBox choice(&authWindow);
                choice.setWindowTitle("Existing financial data found");
                choice.setText("PFBM found financial records from before user profiles were added.");
                choice.setInformativeText(
                    "Import those records into this profile? The original database will be kept. "
                    "This one-time choice determines which profile receives the old records.");
                QPushButton *importButton = choice.addButton(
                    "Import into this profile", QMessageBox::AcceptRole);
                QPushButton *freshButton = choice.addButton(
                    "Start with empty data", QMessageBox::DestructiveRole);
                choice.addButton(QMessageBox::Cancel);
                choice.setDefaultButton(importButton);
                choice.exec();

                if (choice.clickedButton() == importButton) {
                    importLegacy = true;
                } else if (choice.clickedButton() == freshButton) {
                    if (!users.setMetadataValue(
                            claimKey, QStringLiteral("skipped:%1").arg(user->id()), pathError)) {
                        authService.logout();
                        QMessageBox::critical(&authWindow, "Choice could not be saved",
                                              "PFBM could not record this one-time choice. "
                                              "Sign-in was stopped to protect the existing data.");
                        authWindow.resetToLogin();
                        return;
                    }
                } else {
                    // Keep the pending claim so the same profile can decide later.
                    authService.logout();
                    authWindow.resetToLogin();
                    return;
                }
            }
        }

        auto candidate = std::make_unique<MainWindow>(*user, financePath);
        if (!candidate->isDatabaseReady()) {
            const QString message = candidate->databaseError();
            authService.logout();
            QMessageBox::critical(&authWindow, "Profile data unavailable", message);
            authWindow.resetToLogin();
            return;
        }
        if (importLegacy) {
            QString importError;
            if (!candidate->importLegacyData(legacyPath, importError)) {
                authService.logout();
                QMessageBox::critical(&authWindow, "Import could not be completed", importError);
                authWindow.resetToLogin();
                return;
            }
            if (!users.setMetadataValue(
                    claimKey, QStringLiteral("imported:%1").arg(user->id()), pathError)) {
                authService.logout();
                QMessageBox::critical(&authWindow, "Import status could not be saved",
                                      "The import succeeded for this profile, but PFBM could "
                                      "not record its status. The original database remains available.");
                authWindow.resetToLogin();
                return;
            }
        }

        // Legacy preferences are inherited by the first opened profile only;
        // future profiles receive independent defaults.
        QSettings preferenceSettings;
        for (const QString &key : {
                 QStringLiteral("dashboard/activeAccountId"),
                 QStringLiteral("settings/dateFormat"),
                 QStringLiteral("settings/numberFormat"),
                 QStringLiteral("settings/defaultReportPeriod"),
                 QStringLiteral("settings/importDuplicatePolicy")}) {
            preferenceSettings.remove(key);
        }

        mainWindow = std::move(candidate);
        QObject::connect(mainWindow.get(), &MainWindow::logoutRequested, &app, [&]() {
            if (mainWindow) {
                mainWindow->hide();
                mainWindow.reset(); // closes the profile database and clears cached records
            }
            authService.logout();
            authWindow.resetToLogin();
            authWindow.refreshTheme();
            authWindow.show();
            authWindow.raise();
            authWindow.activateWindow();
        });
        authWindow.hide();
        mainWindow->show();
    });

    authWindow.show();
    return app.exec();
}
