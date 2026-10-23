#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <sqlite3.h>
#include <string>

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    // Prevent accidental copying of the database connection.
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool open(const std::string& path, std::string& error);
    void close() noexcept;

    bool isOpen() const noexcept;

    // Execute SQL statements that do not require bound parameters.
    bool execute(const std::string& sql, std::string& error);

    // Create the application's database tables.
    bool initializeSchema(std::string& error);

    // Used by repositories to execute prepared statements.
    sqlite3* handle() const noexcept;

private:
    sqlite3* m_db;
};

#endif // DATABASEMANAGER_H
