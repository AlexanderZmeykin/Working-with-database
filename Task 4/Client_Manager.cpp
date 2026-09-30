#include "client_manager.h"
#include <cstring>
#include <stdexcept>
#include <memory>

using detail::BindValue;

// Проверяем версию MySQL и меняем тип данных в mysql_bind_bool_t в зависимости от того, что у нас за версия. 
#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 80000
using mysql_bind_bool_t = bool;
#else
using mysql_bind_bool_t = my_bool;
#endif

namespace 
{
    struct MysqlResDeleter 
    {
        void operator()(MYSQL_RES* r) const { if (r) mysql_free_result(r); }
    };
    using MysqlResPtr = std::unique_ptr<MYSQL_RES, MysqlResDeleter>;

    struct StmtGuard 
    {
        MYSQL_STMT* s = nullptr;
        explicit StmtGuard(MYSQL_STMT* p) : s(p) {}
        ~StmtGuard() { if (s) mysql_stmt_close(s); }
        StmtGuard(const StmtGuard&) = delete;
        StmtGuard& operator=(const StmtGuard&) = delete;
    };

    // Защита от SQL-иньекции через Prepare statement-ы. 
    long long runPrepared(MYSQL* conn,
        const std::string& sql,
        const std::vector<BindValue>& values) 
    {
        MYSQL_STMT* raw = mysql_stmt_init(conn);
        if (!raw) throw std::runtime_error("mysql_stmt_init failed");
        StmtGuard stmt(raw);

        if (mysql_stmt_prepare(stmt.s, sql.c_str(), sql.size()) != 0)
            throw std::runtime_error(std::string("prepare: ") + mysql_stmt_error(stmt.s));

        std::vector<MYSQL_BIND>      binds(values.size());
        std::vector<std::string>     strs(values.size());
        std::vector<unsigned long>   lens(values.size(), 0);
        std::vector<mysql_bind_bool_t> nulls(values.size(), 0);
        std::vector<long long>       ints(values.size(), 0);

        std::memset(binds.data(), 0, binds.size() * sizeof(MYSQL_BIND));

        for (size_t i = 0; i < values.size(); ++i) 
        {
            const auto& v = values[i];
            binds[i].is_null = &nulls[i];

            switch (v.kind) 
            {
            case BindValue::STR:
                strs[i] = v.s;
                lens[i] = strs[i].size();
                binds[i].buffer_type = MYSQL_TYPE_STRING;
                binds[i].buffer = strs[i].data();      
                binds[i].buffer_length = lens[i];
                binds[i].length = &lens[i];
                break;
            case BindValue::INT:
                ints[i] = v.i;
                binds[i].buffer_type = MYSQL_TYPE_LONGLONG;
                binds[i].buffer = &ints[i];
                break;
            case BindValue::NUL:
                nulls[i] = 1;
                binds[i].buffer_type = MYSQL_TYPE_NULL;
                break;
            }
        }

        if (mysql_stmt_bind_param(stmt.s, binds.data()) != 0)
            throw std::runtime_error(std::string("bind: ") + mysql_stmt_error(stmt.s));
        if (mysql_stmt_execute(stmt.s) != 0)
            throw std::runtime_error(std::string("execute: ") + mysql_stmt_error(stmt.s));

        return static_cast<long long>(mysql_stmt_insert_id(stmt.s));
    }
}

ClientManager::ClientManager(const DBConfig& cfg) : cfg_(cfg) 
{
    conn_ = mysql_init(nullptr);
    if (!conn_) throw std::runtime_error("mysql_init failed");

    if (!mysql_real_connect(conn_,
        cfg_.host.c_str(),
        cfg_.user.c_str(),
        cfg_.password.c_str(),
        nullptr,
        cfg_.port,
        nullptr, 0)) 
    {
        std::string err = mysql_error(conn_);
        mysql_close(conn_);
        throw std::runtime_error("MySQL connect: " + err);
    }

    mysql_set_character_set(conn_, "utf8mb4");

    exec("CREATE DATABASE IF NOT EXISTS `" + cfg_.database +
        "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci");

    if (mysql_select_db(conn_, cfg_.database.c_str()) != 0) 
    {
        std::string err = mysql_error(conn_);
        mysql_close(conn_);
        throw std::runtime_error("USE db: " + err);
    }
}

ClientManager::~ClientManager() 
{
    if (conn_) mysql_close(conn_);
}

void ClientManager::exec(const std::string& sql) const 
{
    if (mysql_real_query(conn_, sql.c_str(), sql.size()) != 0) 
    {
        throw std::runtime_error("SQL error: " + std::string(mysql_error(conn_)) +
            "\nSQL: " + sql);
    }
}

std::string ClientManager::escape(const std::string& s) const 
{
    std::vector<char> buf(s.size() * 2 + 1);
    unsigned long n = mysql_real_escape_string(conn_, buf.data(), s.c_str(), s.size());
    return std::string(buf.data(), n);
}

void ClientManager::createTables() 
{
    exec(R"SQL(
        CREATE TABLE IF NOT EXISTS clients (
            id         BIGINT AUTO_INCREMENT PRIMARY KEY,
            first_name VARCHAR(255) NOT NULL,
            last_name  VARCHAR(255) NOT NULL,
            email      VARCHAR(255) NULL,
            UNIQUE KEY uk_clients_email (email)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )SQL");

    exec(R"SQL(
        CREATE TABLE IF NOT EXISTS phones (
            id        BIGINT AUTO_INCREMENT PRIMARY KEY,
            client_id BIGINT NOT NULL,
            phone     VARCHAR(64) NOT NULL,
            KEY idx_phones_client (client_id),
            KEY idx_phones_phone  (phone),
            CONSTRAINT fk_phones_client
                FOREIGN KEY (client_id) REFERENCES clients(id)
                ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )SQL");
}

long long ClientManager::addClient(const std::string& first_name,
    const std::string& last_name,
    const std::string& email) 
{
    std::vector<BindValue> params = 
    {
        BindValue::str(first_name),
        BindValue::str(last_name),
        email.empty() ? BindValue::nul() : BindValue::str(email)
    };
    return runPrepared(conn_,
        "INSERT INTO clients(first_name, last_name, email) VALUES(?,?,?)",
        params);
}

long long ClientManager::addPhone(long long client_id, const std::string& phone) 
{
    std::vector<BindValue> params = 
    {
        BindValue::num(client_id),
        BindValue::str(phone)
    };
    return runPrepared(conn_,
        "INSERT INTO phones(client_id, phone) VALUES(?,?)",
        params);
}

void ClientManager::updateClient(long long client_id,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email) 
{
    std::string sql = "UPDATE clients SET ";
    std::vector<std::string>    sets;
    std::vector<BindValue>      params;

    if (!first_name.empty()) { sets.push_back("first_name = ?"); params.push_back(BindValue::str(first_name)); }
    if (!last_name.empty()) { sets.push_back("last_name  = ?"); params.push_back(BindValue::str(last_name)); }
    if (!email.empty()) { sets.push_back("email      = ?"); params.push_back(BindValue::str(email)); }

    if (sets.empty()) return;  

    for (size_t i = 0; i < sets.size(); ++i) 
    {
        if (i) sql += ", ";
        sql += sets[i];
    }
    sql += " WHERE id = ?";
    params.push_back(BindValue::num(client_id));

    runPrepared(conn_, sql, params);
}

void ClientManager::deletePhone(long long phone_id) 
{
    std::vector<BindValue> params = { BindValue::num(phone_id) };
    runPrepared(conn_, "DELETE FROM phones WHERE id = ?", params);
}

void ClientManager::deleteClient(long long client_id) 
{
    std::vector<BindValue> params = { BindValue::num(client_id) };
    runPrepared(conn_, "DELETE FROM clients WHERE id = ?", params);
}

std::vector<Client> ClientManager::collectClients(MYSQL_RES* res) const 
{
    std::vector<Client> result;
    if (!res) return result;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) 
    {
        Client c;
        c.id = row[0] ? std::stoll(row[0]) : 0;
        c.first_name = row[1] ? row[1] : "";
        c.last_name = row[2] ? row[2] : "";
        c.email = row[3] ? row[3] : "";
        result.push_back(std::move(c));
    }
    return result;
}

void ClientManager::attachPhones(std::vector<Client>& clients) const 
{
    for (auto& c : clients) 
    {
        std::string sql = "SELECT id, phone FROM phones WHERE client_id = " +
            std::to_string(c.id);
        if (mysql_real_query(conn_, sql.c_str(), sql.size()) != 0)
            throw std::runtime_error(std::string("attachPhones: ") + mysql_error(conn_));

        MysqlResPtr res(mysql_store_result(conn_));
        if (!res) continue;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res.get())) != nullptr) 
        {
            Phone p;
            p.id = row[0] ? std::stoll(row[0]) : 0;
            p.client_id = c.id;
            p.phone = row[1] ? row[1] : "";
            c.phones.push_back(std::move(p));
        }
    }
}

std::vector<Client> ClientManager::findClients(const std::string& query) const 
{
    std::string pattern = "%" + escape(query) + "%";

    std::string sql =
        "SELECT DISTINCT c.id, c.first_name, c.last_name, c.email "
        "FROM clients c "
        "LEFT JOIN phones p ON p.client_id = c.id "
        "WHERE c.first_name LIKE '" + pattern + "' "
        "   OR c.last_name  LIKE '" + pattern + "' "
        "   OR c.email      LIKE '" + pattern + "' "
        "   OR p.phone      LIKE '" + pattern + "'";

    if (mysql_real_query(conn_, sql.c_str(), sql.size()) != 0)
        throw std::runtime_error(std::string("findClients: ") + mysql_error(conn_));

    MysqlResPtr res(mysql_store_result(conn_));
    auto clients = collectClients(res.get());
    attachPhones(clients);
    return clients;
}

std::optional<Client> ClientManager::getClient(long long client_id) const 
{
    std::string sql =
        "SELECT id, first_name, last_name, email "
        "FROM clients WHERE id = " + std::to_string(client_id);

    if (mysql_real_query(conn_, sql.c_str(), sql.size()) != 0)
        throw std::runtime_error(std::string("getClient: ") + mysql_error(conn_));

    MysqlResPtr res(mysql_store_result(conn_));
    auto clients = collectClients(res.get());
    if (clients.empty()) return std::nullopt;

    attachPhones(clients);
    return clients.front();
}

std::vector<Client> ClientManager::getAllClients() const 
{
    std::string sql =
        "SELECT id, first_name, last_name, email FROM clients ORDER BY id";

    if (mysql_real_query(conn_, sql.c_str(), sql.size()) != 0)
        throw std::runtime_error(std::string("getAllClients: ") + mysql_error(conn_));

    MysqlResPtr res(mysql_store_result(conn_));
    auto clients = collectClients(res.get());
    attachPhones(clients);
    return clients;
}