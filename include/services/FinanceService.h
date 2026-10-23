#ifndef FINANCESERVICE_H
#define FINANCESERVICE_H

#include "repositories/AccountRepository.h"
#include "repositories/TransactionRepository.h"

#include <cstdint>
#include <optional>
#include <string>

class FinanceService
{
public:
    FinanceService(
        AccountRepository& accountRepository,
        TransactionRepository& transactionRepository
    );

    // Calculate an account's current balance.
    // Opening balance + income - expenses.
    std::optional<std::int64_t> calculateAccountBalancePaise(
        int accountId,
        std::string& error
    ) const;

    // Calculate total income within an inclusive date range.
    // Dates must use YYYY-MM-DD format.
    std::optional<std::int64_t> calculateTotalIncomePaise(
        const std::string& startDate,
        const std::string& endDate,
        std::string& error
    ) const;

    // Calculate total expenses within an inclusive date range.
    std::optional<std::int64_t> calculateTotalExpensesPaise(
        const std::string& startDate,
        const std::string& endDate,
        std::string& error
    ) const;

    // Calculate income minus expenses within a date range.
    std::optional<std::int64_t> calculateNetSavingsPaise(
        const std::string& startDate,
        const std::string& endDate,
        std::string& error
    ) const;

private:
    AccountRepository& m_accountRepository;
    TransactionRepository& m_transactionRepository;
};

#endif // FINANCESERVICE_H
