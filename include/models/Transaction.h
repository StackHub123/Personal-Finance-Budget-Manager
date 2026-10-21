#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <cstdint>
#include <string>

enum class TransactionType
{
    Income,
    Expense
};

class Transaction
{
public:
    Transaction(int id,
                int accountId,
                std::int64_t amountPaise,
                TransactionType type,
                std::string category,
                std::string description,
                std::string date);

    int getId() const noexcept;
    int getAccountId() const noexcept;
    std::int64_t getAmountPaise() const noexcept;
    TransactionType getType() const noexcept;
    const std::string& getCategory() const noexcept;
    const std::string& getDescription() const noexcept;
    const std::string& getDate() const noexcept;

    void setId(int id);
    void setAccountId(int accountId);
    void setAmountPaise(std::int64_t amount);
    void setType(TransactionType type);
    void setCategory(std::string category);
    void setDescription(std::string description);
    void setDate(std::string date);

private:
    int m_id;
    int m_accountId;
    std::int64_t m_amountPaise;
    TransactionType m_type;
    std::string m_category;
    std::string m_description;
    std::string m_date;
};

#endif // TRANSACTION_H
