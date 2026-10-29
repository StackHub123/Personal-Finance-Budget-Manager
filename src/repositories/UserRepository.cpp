#include "repositories/UserRepository.h"

#include <sqlite3.h>

#include <memory>

namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

bool prepare(sqlite3 *db, const char *sql, Statement &statement, QString &error)
{
    sqlite3_stmt *raw = nullptr;
    const int result = sqlite3_prepare_v2(db, sql, -1, &raw, nullptr);
    statement.reset(raw);
    if (result != SQLITE_OK) {
        error = QStringLiteral("Authentication storage is unavailable.");
        return false;
    }
    return true;
}

QString columnText(sqlite3_stmt *statement, int column)
{
    const auto *text = sqlite3_column_text(statement, column);
    return text ? QString::fromUtf8(reinterpret_cast<const char *>(text)) : QString();
}

void bindText(sqlite3_stmt *statement, int index, const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    sqlite3_bind_text(statement, index, utf8.constData(), utf8.size(), SQLITE_TRANSIENT);
}
}

UserRepository::UserRepository(DatabaseManager &database)
    : m_database(database)
{
}

bool UserRepository::initializeSchema(QString &error)
{
    if (!m_database.isOpen()) {
        error = QStringLiteral("Authentication storage is unavailable.");
        return false;
    }
    const char *schema = R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            email TEXT NOT NULL COLLATE NOCASE UNIQUE,
            password_hash TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS app_metadata (
            key TEXT PRIMARY KEY NOT NULL,
            value TEXT NOT NULL
        );
    )SQL";
    std::string dbError;
    if (!m_database.execute(schema, dbError)) {
        error = QStringLiteral("Authentication storage could not be initialized.");
        return false;
    }
    error.clear();
    return true;
}

bool UserRepository::createUser(const QString &canonicalEmail,
                                const QString &passwordHash,
                                int &userId,
                                QString &error)
{
    userId = 0;
    Statement statement(nullptr, sqlite3_finalize);
    if (!prepare(m_database.handle(),
                 "INSERT INTO users (email, password_hash) VALUES (?, ?);",
                 statement, error))
        return false;

    bindText(statement.get(), 1, canonicalEmail);
    bindText(statement.get(), 2, passwordHash);
    const int result = sqlite3_step(statement.get());
    if (result != SQLITE_DONE) {
        const int extended = sqlite3_extended_errcode(m_database.handle());
        error = (extended == SQLITE_CONSTRAINT_UNIQUE ||
                 extended == SQLITE_CONSTRAINT_PRIMARYKEY)
            ? QStringLiteral("An account with this email address already exists.")
            : QStringLiteral("Your account could not be created. Please try again.");
        return false;
    }
    userId = static_cast<int>(sqlite3_last_insert_rowid(m_database.handle()));
    error.clear();
    return userId > 0;
}

std::optional<UserCredential> UserRepository::findByEmail(
    const QString &canonicalEmail, QString &error) const
{
    Statement statement(nullptr, sqlite3_finalize);
    if (!prepare(m_database.handle(),
                 "SELECT id, email, password_hash FROM users WHERE email = ? LIMIT 1;",
                 statement, error))
        return std::nullopt;
    bindText(statement.get(), 1, canonicalEmail);
    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        error.clear();
        return std::nullopt;
    }
    if (result != SQLITE_ROW) {
        error = QStringLiteral("Authentication storage is unavailable.");
        return std::nullopt;
    }
    UserCredential credential;
    credential.id = sqlite3_column_int(statement.get(), 0);
    credential.email = columnText(statement.get(), 1);
    credential.passwordHash = columnText(statement.get(), 2);
    error.clear();
    return credential;
}

std::optional<QString> UserRepository::metadataValue(
    const QString &key, QString &error) const
{
    Statement statement(nullptr, sqlite3_finalize);
    if (!prepare(m_database.handle(),
                 "SELECT value FROM app_metadata WHERE key = ? LIMIT 1;",
                 statement, error))
        return std::nullopt;
    bindText(statement.get(), 1, key);
    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        error.clear();
        return std::nullopt;
    }
    if (result != SQLITE_ROW) {
        error = QStringLiteral("Authentication storage is unavailable.");
        return std::nullopt;
    }
    const QString value = columnText(statement.get(), 0);
    error.clear();
    return value;
}

bool UserRepository::insertMetadataIfAbsent(const QString &key,
                                            const QString &value,
                                            bool &inserted,
                                            QString &error)
{
    inserted = false;
    Statement statement(nullptr, sqlite3_finalize);
    if (!prepare(m_database.handle(),
                 "INSERT OR IGNORE INTO app_metadata (key, value) VALUES (?, ?);",
                 statement, error))
        return false;
    bindText(statement.get(), 1, key);
    bindText(statement.get(), 2, value);
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        error = QStringLiteral("Authentication storage is unavailable.");
        return false;
    }
    inserted = sqlite3_changes(m_database.handle()) == 1;
    error.clear();
    return true;
}

bool UserRepository::setMetadataValue(const QString &key,
                                      const QString &value,
                                      QString &error)
{
    Statement statement(nullptr, sqlite3_finalize);
    if (!prepare(m_database.handle(),
                 "INSERT INTO app_metadata (key, value) VALUES (?, ?) "
                 "ON CONFLICT(key) DO UPDATE SET value = excluded.value;",
                 statement, error))
        return false;
    bindText(statement.get(), 1, key);
    bindText(statement.get(), 2, value);
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        error = QStringLiteral("Authentication storage is unavailable.");
        return false;
    }
    error.clear();
    return true;
}
