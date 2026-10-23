#ifndef BUDGET_H
#define BUDGET_H

#include <cstdint>
#include <string>

class Budget
{
public:
    Budget(int id,
           std::string category,
           int month,
           int year,
           std::int64_t limitPaise);

    int getId() const noexcept;
    const std::string& getCategory() const noexcept;
    int getMonth() const noexcept;
    int getYear() const noexcept;
    std::int64_t getLimitPaise() const noexcept;

    void setId(int id);
    void setCategory(std::string category);
    void setMonth(int month);
    void setYear(int year);
    void setLimitPaise(std::int64_t limitPaise);

private:
    int m_id;
    std::string m_category;
    int m_month;
    int m_year;
    std::int64_t m_limitPaise;
};

#endif // BUDGET_H
