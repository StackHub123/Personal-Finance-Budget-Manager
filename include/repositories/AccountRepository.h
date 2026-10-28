#ifndef ACCOUNTREPOSITORY_H
#define ACCOUNTREPOSITORY_H

#include "database/DatabaseManager.h"
#include "models/Account.h"

#include <optional>
#include <string>
#include <vector>

class AccountRepository
{
public:
    explicit AccountRepository(DatabaseManager& database);

    // Save an account and assign its generated database ID.
    bool add(Account& account, std::string& error);

    // Retrieve an account using its ID.
    std::optional<Account> findById(
        int id,
        std::string& error
    ) const;

    // Retrieve all accounts.
    std::vector<Account> getAll(std::string& error) const;

    // Update an existing account.
    bool update(const Account& account, std::string& error);

    // Delete an account that has no associated transactions.
    bool remove(int id, std::string& error);

    // Restore an account with its original database ID.
    bool addRestored(const Account& account, std::string& error);

private:
    DatabaseManager& m_database;
};

#endif // ACCOUNTREPOSITORY_H
