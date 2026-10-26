#ifndef TRANSACTIONSERVICE_H
#define TRANSACTIONSERVICE_H

#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"
#include "models/Transaction.h"

#include <cstdint>
#include <string>
#include <vector>

class TransactionService
{
public:
    TransactionService(
        AccountRepository& accountRepository,
        TransactionRepository& transactionRepository
    );

    // Create and save an expense.
    bool addExpense(
        int accountId,
        std::int64_t amountPaise,
        std::string category,
        std::string description,
        std::string date,
        int& createdTransactionId,
        std::string& error
    );

    // Create and save an income transaction.
    bool addIncome(
        int accountId,
        std::int64_t amountPaise,
        std::string category,
        std::string description,
        std::string date,
        int& createdTransactionId,
        std::string& error
    );

    // Retrieve all stored transactions.
    std::vector<Transaction> getAllTransactions(
        std::string& error
    ) const;

    // Update an existing transaction.
    bool updateTransaction(
        const Transaction& transaction,
        std::string& error
    );

    // Delete a transaction.
    bool deleteTransaction(
        int transactionId,
        std::string& error
    );

private:
    // Shared processing path for income and expenses.
    bool addTransaction(
        int accountId,
        std::int64_t amountPaise,
        TransactionType type,
        std::string category,
        std::string description,
        std::string date,
        int& createdTransactionId,
        std::string& error
    );

    AccountRepository& m_accountRepository;
    TransactionRepository& m_transactionRepository;
};

#endif // TRANSACTIONSERVICE_H
