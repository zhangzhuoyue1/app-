#pragma once
#include "MySQLPool.h"
#include <string>
#include <optional>
#include <map>

struct UserInfo 
{
    std::string name;
    std::string email;
    std::string type;  // 用户类型
};

class MySQLDAO {
public:
    MySQLDAO(const std::string& url, const std::string& user,
             const std::string& pass, const std::string& schema, int poolSize);
    MySQLDAO();

    ~MySQLDAO();

    // 注册用户（返回 1 成功，0 重复用户名/邮箱，-1 出错）
    int reg_user(const std::string& name, const std::string& email, const std::string& pwd);

    // 删除用户
    int del_user_by_name(const std::string& name);

    // 修改密码
    int update_user_pwd(const std::string& name, const std::string& new_pwd);

    // 查询用户信息（返回用户名、邮箱、用户类型等）
    UserInfo get_user_by_name(const std::string& name);

private:
    std::unique_ptr<MySqlPool> m_pool;
};
