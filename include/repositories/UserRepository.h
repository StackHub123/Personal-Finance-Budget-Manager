#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include "database/DatabaseManager.h"

#include <QString>
#include <optional>

struct UserCredential
{
    int id = 0;
    QString email;
    QString passwordHash;
};

class UserRepository final
{
public:
    explicit UserRepository(DatabaseManager &database);

    bool initializeSchema(QString &error);
    bool createUser(const QString &canonicalEmail,
                    const QString &passwordHash,
                    int &userId,
                    QString &error);
    std::optional<UserCredential> findByEmail(const QString &canonicalEmail,
                                               QString &error) const;
    std::optional<QString> metadataValue(const QString &key,
                                         QString &error) const;
    bool insertMetadataIfAbsent(const QString &key,
                               const QString &value,
                               bool &inserted,
                               QString &error);
    bool setMetadataValue(const QString &key,
                          const QString &value,
                          QString &error);

private:
    DatabaseManager &m_database;
};

#endif // USERREPOSITORY_H
