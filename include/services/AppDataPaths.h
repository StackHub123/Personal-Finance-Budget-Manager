#ifndef APPDATAPATHS_H
#define APPDATAPATHS_H

#include <QString>

namespace AppDataPaths {
QString dataDirectory();
QString authenticationDatabase();
QString legacyFinancialDatabase();
QString financialDatabaseForUser(int userId,
                                 const QString &baseDirectory = QString());
bool ensureFinancialDatabaseDirectory(int userId,
                                      const QString &baseDirectory,
                                      QString &databasePath,
                                      QString &error);
}

#endif // APPDATAPATHS_H
