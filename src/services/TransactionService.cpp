#include "services/TransactionService.h"

#include <exception>
#include <utility>

TransactionService::TransactionService(
    AccountRepository& accountRepository,
    TransactionRepository& transactionRepository)
    : m_accountRepository(accountRepository),
      m_transactionRepository(transactionRepository)
{
}

bool TransactionService::addExpense(
    int accountId,
    std::int64_t amountPaise,
    std::string category,
    std::string description,
    std::string date,
    int& createdTransactionId,
    std::string& error)
{
    return addTransaction(
        accountId,
        amountPaise,
        TransactionType::Expense,
        std::move(category),
        std::move(description),
        std::move(date),
        createdTransactionId,
        error
    );
}

bool TransactionService::addIncome(
    int accountId,
    std::int64_t amountPaise,
    std::string category,
    std::string description,
    std::string date,
    int& createdTransactionId,
    std::string& error)
{
    return addTransaction(
        accountId,
        amountPaise,
        TransactionType::Income,
        std::move(category),
        std::move(description),
        std::move(date),
        createdTransactionId,
        error
    );
}

bool TransactionService::addTransaction(
    int accountId,
    std::int64_t amountPaise,
    TransactionType type,
    std::string category,
    std::string description,
    std::string date,
    int& createdTransactionId,
    std::string& error)
{
    createdTransactionId = 0;

    // A transaction must belong to an existing account.
    const auto account = m_accountRepository.findById(accountId, error);

    if (!account)
    {
        if (error.empty())
        {
            error = "Cannot create transaction: account not found.";
        }

        return false;
    }

    try
    {
        // The Transaction constructor validates its fields.
        Transaction transaction(
            0,
            accountId,
            amountPaise,
            type,
            std::move(category),
            std::move(description),
            std::move(date)
        );

        // The repository stores the transaction in SQLite and assigns its ID.
        if (!m_transactionRepository.add(transaction, error))
        {
            return false;
        }

        createdTransactionId = transaction.getId();
        error.clear();

        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}

std::vector<Transaction> TransactionService::getAllTransactions(
    std::string& error) const
{
    return m_transactionRepository.getAll(error);
}

bool TransactionService::updateTransaction(
    const Transaction& transaction,
    std::string& error)
{
    if (transaction.getId() <= 0)
    {
        error = "Cannot update transaction: transaction ID must be positive.";
        return false;
    }

    // Ensure the replacement account exists.
    const auto account =
        m_accountRepository.findById(transaction.getAccountId(), error);

    if (!account)
    {
        if (error.empty())
        {
            error = "Cannot update transaction: account not found.";
        }

        return false;
    }

    return m_transactionRepository.update(transaction, error);
}

bool TransactionService::deleteTransaction(
    int transactionId,
    std::string& error)
{
    return m_transactionRepository.remove(transactionId, error);
}
