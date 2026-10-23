#ifndef TRANSACTIONREPOSITORY_H
#define TRANSACTIONREPOSITORY_H

#include "database/DatabaseManager.h"
#include "models/Transaction.h"

#include <optional>
#include <string>
#include <vector>

class TransactionRepository
{
public:
    explicit TransactionRepository(DatabaseManager& database);

    // Save a new transaction and assign its generated database ID.
    bool add(Transaction& transaction, std::string& error);

    // Retrieve one transaction by ID.
    std::optional<Transaction> findById(
        int id,
        std::string& error
    ) const;

    // Retrieve all transactions.
    std::vector<Transaction> getAll(std::string& error) const;

    // Update an existing transaction.
    bool update(const Transaction& transaction, std::string& error);

    // Delete a transaction by ID.
    bool remove(int id, std::string& error);

private:
    DatabaseManager& m_database;
};

#endif // TRANSACTIONREPOSITORY_H
