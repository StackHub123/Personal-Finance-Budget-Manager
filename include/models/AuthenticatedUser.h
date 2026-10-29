#ifndef AUTHENTICATEDUSER_H
#define AUTHENTICATEDUSER_H

#include <QString>
#include <utility>

class AuthService;

class AuthenticatedUser final
{
public:
    int id() const noexcept { return m_id; }
    const QString &email() const noexcept { return m_email; }
    bool isValid() const noexcept { return m_id > 0 && !m_email.isEmpty(); }

private:
    AuthenticatedUser(int id, QString email)
        : m_id(id), m_email(std::move(email))
    {
    }

    int m_id;
    QString m_email;

    friend class AuthService;
};

#endif // AUTHENTICATEDUSER_H
