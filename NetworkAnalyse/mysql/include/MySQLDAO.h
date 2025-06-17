#pragma once
#include "MySQLPool.h"
#include <string>
#include <optional>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;
struct HttpFlowInfo {
    std::string flow_id;
    int app_uid;
    std::string protocol;
    std::string top_protocol;
    std::string src_ip;
    std::string http_version; // HTTP/1.1, HTTP/2, etc.
    int src_port;
    std::string dst_ip;
    int dst_port;
    std::string host;
    std::string url;
    std::string method;
    int status_code;
    std::string content_type;
    std::string start_time;
};
struct HttpPacket {
    std::string flow_id;
    std::string type; // request / response
    nlohmann::json headers;
    std::string top_protocol;
    std::string body;
    std::string timestamp;
    std::string content_type; // Content-Type header
    int length; 
};

struct UserInfo 
{
    std::string name;
    std::string email;
    std::string type;  // 用户类型
    std::string pwd;   // 密码
    std::string create_time; // 创建时间
    std::string last_login_time; // 上次登录时间
};

struct SessionInfo 
{
    int app_uid;
    std::string timestamp;
    std::string session_id;
    std::string protocol;
    std::string src_ip;
    int src_port;
    std::string dst_ip;
    int dst_port;
    int size;
    std::time_t last_update_time; // 最后更新时间
};

class MySQLDAO {
public:
    MySQLDAO(const std::string& url, const std::string& user,
             const std::string& pass, const std::string& schema, int poolSize);
    MySQLDAO();

    ~MySQLDAO();

    // 注册用户（返回 1 成功，0 重复用户名/邮箱，-1 出错）
    int                 reg_user(const std::string& name, const std::string& email, const std::string& pwd);
    int                 del_user_by_name(const std::string& name);    // 删除用户
    int                 update_user(const UserInfo& user);   // 修改用户信息（基于用户名）
    UserInfo            get_user_by_name(const std::string& name);// 查询用户信息（返回用户名、邮箱、用户类型等）
    int                 store_app_info_if_not_exists(int app_uid, const std::string& package_name);
    int                 add_user_with_role(const std::string& name, const std::string& email, const std::string& pwd, const std::string& role);

    //存储报文
    int                 store_tcp(const json& j);
    int                 store_udp(const  json& j);
    int                 store_icmp(const json& j);
    int                 store_http(const json& j);
    int                 store_dns(const json& j);

    //存储会话
    int                 insert_or_update_session_info(const SessionInfo& session);
    // int                 insert_session_resource(const std::string& session_id, const std::string& resource);
    // int                 insert_session_packet(const std::string& session_id, const json& packet);
    
    //存储请求响应报文
    bool                 insert_http_flow_info(const HttpFlowInfo& info);
    bool                 insert_http_packet(const HttpPacket& pkt);
private:
    std::unique_ptr<MySqlPool> m_pool;
};
