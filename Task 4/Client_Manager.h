#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mysql.h>   

struct Phone 
{
    long long   id = 0;
    long long   client_id = 0;
    std::string phone;
};

struct Client 
{
    long long   id = 0;
    std::string first_name;
    std::string last_name;
    std::string email;
    std::vector<Phone> phones;
};

struct DBConfig 
{
    std::string  host = "127.0.0.1";
    unsigned int port = 3306;
    std::string  user = "root";
    std::string  password = "";
    std::string  database = "clients_db";
};

class ClientManager 
{
public:
    explicit ClientManager(const DBConfig& cfg);
    ~ClientManager();

    ClientManager(const ClientManager&) = delete;
    ClientManager& operator=(const ClientManager&) = delete;

    void createTables();

    long long addClient(const std::string& first_name,
        const std::string& last_name,
        const std::string& email);

    long long addPhone(long long client_id, const std::string& phone);

    void updateClient(long long client_id,
        const std::string& first_name = "",
        const std::string& last_name = "",
        const std::string& email = "");

    void deletePhone(long long phone_id);

    void deleteClient(long long client_id);

    std::vector<Client>   findClients(const std::string& query) const;

    std::optional<Client> getClient(long long client_id) const;
    std::vector<Client>   getAllClients() const;

private:
    MYSQL* conn_ = nullptr;
    DBConfig cfg_;

    void        exec(const std::string& sql) const;
    std::string escape(const std::string& s) const;
    std::vector<Client> collectClients(MYSQL_RES* res) const;
    void        attachPhones(std::vector<Client>& clients) const;
};

namespace detail 
{
    struct BindValue 
    {
        enum Kind { STR, INT, NUL } kind;
        std::string s;
        long long   i = 0;
        static BindValue str(const std::string& v) { return { STR, v, 0 }; }
        static BindValue num(long long v) { return { INT, "", v }; }
        static BindValue nul() { return { NUL, "", 0 }; }
    };
}
