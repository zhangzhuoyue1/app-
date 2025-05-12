#include "../include/LogicSystem.h"
#include "../include/HttpConnection.h"
#include <spdlog/spdlog.h>



LogicSystem::LogicSystem() 
{
    reg_post("/user_register", [this](std::shared_ptr<HttpConnection> connection) {
    // 获取请求体
    auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
    spdlog::info("user_register: Received body: {}", body_str);
    
    // 设置响应头
    connection->m_response.set(http::field::content_type, "application/json");
    
    json root;
    json src_root;
    
    try {
        // 解析JSON
        src_root = json::parse(body_str);
        auto email = src_root["email"].get<std::string>();
        auto name = src_root["user"].get<std::string>();
        auto pwd = src_root["passwd"].get<std::string>();
        auto confirm = src_root["confirm"].get<std::string>();


        
        // 查找数据库判断用户是否存在
        int ret =m_mysql.reg_user(name, email, pwd);
        if (ret == 0) 
        {
            root["error"] = 1;
            root["msg"] = "用户名或邮箱已存在";
            spdlog::error("user_register: user {} exits ", name);
        } 
        else if(ret==-1)
        {
            root["error"] = 2;
            root["msg"] = "注册失败";
            spdlog::error("user_register: user {} register failed", name);
        }
        else
        {
            root["error"] = 0;
            root["email"] = src_root["email"];
            root["user"] = src_root["user"].get<std::string>();
            root["passwd"] = src_root["passwd"].get<std::string>(); 
            spdlog::info("user_register: user {} register success", name);  
        } 
        
    } 
    catch (const json::parse_error& e) 
    {
        spdlog::error("Parse error in JSON: {}", e.what());
        root["error"] = ErrorCodes::Error_Json;
    } 
    catch (const json::type_error& e) 
    {
        spdlog::error("Type error in JSON: {}", e.what());
        root["error"] = ErrorCodes::Error_Json;
    }

    // 发送响应
    std::string jsonstr = root.dump();
    beast::ostream(connection->m_response.body()) << jsonstr;
    spdlog::info("user_register: response: {}", jsonstr);
    
    return true;
});

reg_post("/user_login", [this](std::shared_ptr<HttpConnection> connection) {
    // 获取请求体
    auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
    spdlog::info("user_login: Received body: {}", body_str);
    
    // 设置响应头
    connection->m_response.set(http::field::content_type, "application/json");
    
    //响应数据
    json root;
    //接收到的json数据
    json src_root;  
    
    try {
        // 解析JSON
        src_root = json::parse(body_str);
        auto name = src_root["user"].get<std::string>();
        auto pwd = src_root["passwd"].get<std::string>();

        // 查找数据库判断用户是否存在
        UserInfo user_info = m_mysql.get_user_by_name(name);
        if (user_info.name!=name) 
        {
            root["error"] = 1;
            root["msg"] = "用户不存在";
            spdlog::error("user_register: user {} not exits ", name);
        } 
        else
        {
            root["error"] = 0;
            root["user"] = src_root["user"].get<std::string>();
            root["passwd"] = src_root["passwd"].get<std::string>(); 
            spdlog::info("user_register: user {} login success", name);  
        } 
        
    } 
    catch (const json::parse_error& e) 
    {
        spdlog::error("Parse error in JSON: {}", e.what());
        root["error"] = ErrorCodes::Error_Json;
    } 
    catch (const json::type_error& e) 
    {
        spdlog::error("Type error in JSON: {}", e.what());
        root["error"] = ErrorCodes::Error_Json;
    }

    // 发送响应
    std::string jsonstr = root.dump();
    beast::ostream(connection->m_response.body()) << jsonstr;
    spdlog::info("user_register: response: {}", jsonstr);
    
    return true;
});
}

LogicSystem::~LogicSystem() 
{
    m_get_handlers.clear();
    m_post_handlers.clear();
}

void LogicSystem::reg_get(std::string url, HttpHandler handler) 
{
    m_get_handlers.insert(make_pair(url, handler));
}

bool LogicSystem::handle_get(std::string path, std::shared_ptr<HttpConnection> con) 
{
    //检查路径是否存在
    if (m_get_handlers.find(path) == m_get_handlers.end()) {
        return false;
    }

    m_get_handlers[path](con);
    return true;
}

void LogicSystem::reg_post(std::string url, HttpHandler handler) 
{
    m_post_handlers.insert(make_pair(url, handler));
}

bool LogicSystem::handle_post(std::string path, std::shared_ptr<HttpConnection> con) 
{
    //检查路径是否存在
    if (m_post_handlers.find(path) == m_post_handlers.end()) 
    {
        return false;
    }

    m_post_handlers[path](con);
    return true;
}