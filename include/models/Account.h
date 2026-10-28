#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <cstdint>
#include <string>

class Account
{
public:
    Account(int id,
            std::string name,
            std::string type,
            std::int64_t initialBalancePaise);

    int getId() const noexcept;
    const std::string& getName() const noexcept;
    const std::string& getType() const noexcept;
    std::int64_t getInitialBalancePaise() const noexcept;

    void setId(int id);
    void setName(std::string name);
    void setType(std::string type);
    void setInitialBalancePaise(std::int64_t amount) noexcept;

private:
    int m_id;
    std::string m_name;
    std::string m_type;
    std::int64_t m_initialBalancePaise;
};

#endif // ACCOUNT_H
