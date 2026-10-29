#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include "models/AuthenticatedUser.h"
#include "repositories/UserRepository.h"

#include <QString>
#include <optional>

class AuthService final
{
public:
    explicit AuthService(UserRepository &users);

    bool registerUser(const QString &email,
                      const QString &password,
                      const QString &confirmation,
                      QString &error);
    bool authenticate(const QString &email,
                      const QString &password,
                      QString &error);
    void logout() noexcept;
    bool hasAuthenticatedUser() const noexcept;
    std::optional<AuthenticatedUser> currentUser() const;

    static QString normalizeEmail(const QString &email);
    static bool isValidEmail(const QString &email);
    static bool isStrongEnoughPassword(const QString &password);

private:
    UserRepository &m_users;
    std::optional<AuthenticatedUser> m_currentUser;
};

#endif // AUTHSERVICE_H
