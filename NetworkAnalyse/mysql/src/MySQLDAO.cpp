#include "MySQLDAO.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

MySQLDAO::MySQLDAO(const std::string& url="192.168.89.185:3308", const std::string& user="root",
                   const std::string& pass="123456", const std::string& schema="appnetworkanalyse", int poolSize=10)
{
    m_pool = std::make_unique<MySqlPool>(url, user, pass, schema, poolSize);
}

MySQLDAO::MySQLDAO()
{
     m_pool = std::make_unique<MySqlPool>("192.168.89.185:3308", "root", "123456", "appnetworkanalyse", 10);
}

MySQLDAO::~MySQLDAO() {
    m_pool->Close();
}

// 注册用户,返回 1 成功，0 重复用户名/邮箱，-1 出错
int MySQLDAO::reg_user(const std::string& name, const std::string& email, const std::string& pwd) 
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try 
    {
        // 检查用户名或邮箱是否已存在
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT COUNT(*) FROM users WHERE username=? OR email=?"));
        checkStmt->setString(1, name);
        checkStmt->setString(2, email);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        res->next();
        if (res->getInt(1) > 0)
        {
            return 0;  // 用户已存在
        }

        // 插入用户
        std::unique_ptr<sql::PreparedStatement> insertStmt(
            conn->prepareStatement("INSERT INTO users (username, password, email, user_type) VALUES (?, ?, ?, 'user')"));
        insertStmt->setString(1, name);
        insertStmt->setString(2, pwd);  // 实际使用中应为哈希
        insertStmt->setString(3, email);
        insertStmt->execute();
        return 1;
    }
     catch (...) 
    {
        return -1;
    }
    m_pool->return_connection(std::move(conn));
}

// 删除用户,返回 1 成功，0 用户不存在，-1 出错
int MySQLDAO::del_user_by_name(const std::string& name) 
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("DELETE FROM users WHERE username=?"));
        stmt->setString(1, name);
        int rows = stmt->executeUpdate();
        return rows > 0 ? 1 : 0;
    } 
    catch (...) 
    {
        return -1;
    }
    m_pool->return_connection(std::move(conn));
}

// 修改密码
int MySQLDAO::update_user_pwd(const std::string& name, const std::string& new_pwd) 
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("UPDATE users SET password=? WHERE username=?"));
        stmt->setString(1, new_pwd);
        stmt->setString(2, name);
        int rows = stmt->executeUpdate();
        return rows > 0 ? 1 : 0;
    }
     catch (...) 
    {
        return -1;
    }
    m_pool->return_connection(std::move(conn));
}

// 查询用户信息
UserInfo MySQLDAO::get_user_by_name(const std::string& name) 
{
    auto conn = m_pool->get_connection();
    if (!conn)  return UserInfo();

    try 
    {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("SELECT username, email, user_type FROM users WHERE username=?"));
        stmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        if (res->next()) 
        {
            UserInfo user;
            user.name = res->getString("username");
            user.email = res->getString("email");
            user.type = res->getString("user_type");
            return user;
        } 
        else 
        {
            return UserInfo();  // 用户不存在
        }
        // 归还连接
        m_pool->return_connection(std::move(conn));
    } 
    catch (...) 
    {
        return UserInfo(); // 出错或异常
    }
}
