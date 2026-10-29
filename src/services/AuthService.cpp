#include "services/AuthService.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <QRegularExpression>

#include <array>
#include <cstdint>

namespace {
constexpr uint64_t kScryptN = 1u << 15;
constexpr uint64_t kScryptR = 8;
constexpr uint64_t kScryptP = 1;
constexpr uint64_t kScryptMaxMemory = 64u * 1024u * 1024u;
constexpr size_t kSaltSize = 16;
constexpr size_t kHashSize = 32;

bool derivePassword(const QByteArray &password,
                    const QByteArray &salt,
                    std::array<unsigned char, kHashSize> &derived)
{
    return EVP_PBE_scrypt(password.constData(),
                          static_cast<size_t>(password.size()),
                          reinterpret_cast<const unsigned char *>(salt.constData()),
                          static_cast<size_t>(salt.size()),
                          kScryptN, kScryptR, kScryptP, kScryptMaxMemory,
                          derived.data(), derived.size()) == 1;
}

QString makePasswordHash(const QString &password)
{
    QByteArray salt(static_cast<int>(kSaltSize), Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), salt.size()) != 1)
        return {};

    QByteArray passwordBytes = password.toUtf8();
    std::array<unsigned char, kHashSize> derived{};
    const bool success = derivePassword(passwordBytes, salt, derived);
    OPENSSL_cleanse(passwordBytes.data(), static_cast<size_t>(passwordBytes.size()));
    if (!success) {
        OPENSSL_cleanse(derived.data(), derived.size());
        return {};
    }

    const QByteArray saltText = salt.toBase64(QByteArray::Base64UrlEncoding |
                                               QByteArray::OmitTrailingEquals);
    const QByteArray hashText = QByteArray(
        reinterpret_cast<const char *>(derived.data()),
        static_cast<int>(derived.size())).toBase64(QByteArray::Base64UrlEncoding |
                                                   QByteArray::OmitTrailingEquals);
    OPENSSL_cleanse(derived.data(), derived.size());
    return QStringLiteral("scrypt$1$%1$%2$%3$%4$%5")
        .arg(kScryptN).arg(kScryptR).arg(kScryptP)
        .arg(QString::fromLatin1(saltText), QString::fromLatin1(hashText));
}

bool verifyPassword(const QString &password, const QString &encodedHash)
{
    const QStringList parts = encodedHash.split(QLatin1Char('$'));
    if (parts.size() != 7 || parts.at(0) != QStringLiteral("scrypt") ||
        parts.at(1) != QStringLiteral("1") ||
        parts.at(2).toULongLong() != kScryptN ||
        parts.at(3).toULongLong() != kScryptR ||
        parts.at(4).toULongLong() != kScryptP)
        return false;

    const QByteArray salt = QByteArray::fromBase64(
        parts.at(5).toLatin1(), QByteArray::Base64UrlEncoding);
    const QByteArray storedHash = QByteArray::fromBase64(
        parts.at(6).toLatin1(), QByteArray::Base64UrlEncoding);
    if (salt.size() != static_cast<int>(kSaltSize) ||
        storedHash.size() != static_cast<int>(kHashSize))
        return false;

    QByteArray passwordBytes = password.toUtf8();
    std::array<unsigned char, kHashSize> derived{};
    const bool derivedOk = derivePassword(passwordBytes, salt, derived);
    OPENSSL_cleanse(passwordBytes.data(), static_cast<size_t>(passwordBytes.size()));
    if (!derivedOk) {
        OPENSSL_cleanse(derived.data(), derived.size());
        return false;
    }
    const bool matches = CRYPTO_memcmp(derived.data(), storedHash.constData(),
                                       derived.size()) == 0;
    OPENSSL_cleanse(derived.data(), derived.size());
    return matches;
}

void performDummyVerification(const QString &password)
{
    QByteArray passwordBytes = password.toUtf8();
    const QByteArray dummySalt(kSaltSize, '\0');
    std::array<unsigned char, kHashSize> derived{};
    derivePassword(passwordBytes, dummySalt, derived);
    OPENSSL_cleanse(passwordBytes.data(), static_cast<size_t>(passwordBytes.size()));
    OPENSSL_cleanse(derived.data(), derived.size());
}
}

AuthService::AuthService(UserRepository &users)
    : m_users(users)
{
}

QString AuthService::normalizeEmail(const QString &email)
{
    return email.trimmed().toCaseFolded();
}

bool AuthService::isValidEmail(const QString &email)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^[A-Z0-9.!#$%&'*+/=?^_`{|}~-]+@(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+[A-Z]{2,63}$)"),
        QRegularExpression::CaseInsensitiveOption);
    return email.size() <= 254 && pattern.match(email).hasMatch();
}

bool AuthService::isStrongEnoughPassword(const QString &password)
{
    return password.toUcs4().size() >= 8 && password.toUtf8().size() <= 1024 &&
           !password.contains(QChar::Null);
}

bool AuthService::registerUser(const QString &email,
                               const QString &password,
                               const QString &confirmation,
                               QString &error)
{
    m_currentUser.reset();
    const QString canonicalEmail = normalizeEmail(email);
    if (canonicalEmail.isEmpty() || password.isEmpty() || confirmation.isEmpty()) {
        error = QStringLiteral("Enter your email, password, and password confirmation.");
        return false;
    }
    if (!isValidEmail(canonicalEmail)) {
        error = QStringLiteral("Enter a valid email address.");
        return false;
    }
    if (!isStrongEnoughPassword(password)) {
        error = QStringLiteral("Use a password with at least 8 characters.");
        return false;
    }
    if (password != confirmation) {
        error = QStringLiteral("The passwords do not match.");
        return false;
    }

    QString repositoryError;
    const auto existing = m_users.findByEmail(canonicalEmail, repositoryError);
    if (!repositoryError.isEmpty()) {
        error = QStringLiteral("Your account could not be created. Please try again.");
        return false;
    }
    if (existing) {
        error = QStringLiteral("An account with this email address already exists.");
        return false;
    }

    const QString passwordHash = makePasswordHash(password);
    if (passwordHash.isEmpty()) {
        error = QStringLiteral("Password security could not be initialized. Please try again.");
        return false;
    }
    int userId = 0;
    if (!m_users.createUser(canonicalEmail, passwordHash, userId, repositoryError)) {
        error = repositoryError.isEmpty()
            ? QStringLiteral("Your account could not be created. Please try again.")
            : repositoryError;
        return false;
    }
    m_currentUser = AuthenticatedUser(userId, canonicalEmail);
    error.clear();
    return true;
}

bool AuthService::authenticate(const QString &email,
                               const QString &password,
                               QString &error)
{
    m_currentUser.reset();
    const QString canonicalEmail = normalizeEmail(email);
    if (canonicalEmail.isEmpty() || password.isEmpty() ||
        !isValidEmail(canonicalEmail)) {
        error = QStringLiteral("Invalid email or password.");
        return false;
    }

    QString repositoryError;
    const auto credential = m_users.findByEmail(canonicalEmail, repositoryError);
    if (!repositoryError.isEmpty()) {
        error = QStringLiteral("Unable to sign in right now. Please try again.");
        return false;
    }
    if (!credential) {
        performDummyVerification(password);
        error = QStringLiteral("Invalid email or password.");
        return false;
    }
    if (!verifyPassword(password, credential->passwordHash)) {
        error = QStringLiteral("Invalid email or password.");
        return false;
    }

    m_currentUser = AuthenticatedUser(credential->id, credential->email);
    error.clear();
    return true;
}

void AuthService::logout() noexcept
{
    m_currentUser.reset();
}

bool AuthService::hasAuthenticatedUser() const noexcept
{
    return m_currentUser.has_value() && m_currentUser->isValid();
}

std::optional<AuthenticatedUser> AuthService::currentUser() const
{
    return m_currentUser;
}
