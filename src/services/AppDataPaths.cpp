#include "services/AppDataPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace AppDataPaths {
QString dataDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString authenticationDatabase()
{
    return QDir(dataDirectory()).filePath("authentication.sqlite");
}

QString legacyFinancialDatabase()
{
    return QDir(dataDirectory()).filePath("personal_finance.db");
}

QString financialDatabaseForUser(int userId, const QString &baseDirectory)
{
    if (userId <= 0)
        return {};
    const QString base = baseDirectory.isEmpty() ? dataDirectory() : baseDirectory;
    return QDir(QDir(base).filePath(QStringLiteral("users/%1").arg(userId)))
        .filePath("personal_finance.db");
}

bool ensureFinancialDatabaseDirectory(int userId,
                                     const QString &baseDirectory,
                                     QString &databasePath,
                                     QString &error)
{
    databasePath = financialDatabaseForUser(userId, baseDirectory);
    if (databasePath.isEmpty()) {
        error = QStringLiteral("The signed-in profile is invalid.");
        return false;
    }
    if (!QDir().mkpath(QFileInfo(databasePath).absolutePath())) {
        error = QStringLiteral("The profile data folder could not be created.");
        return false;
    }
    error.clear();
    return true;
}
}
