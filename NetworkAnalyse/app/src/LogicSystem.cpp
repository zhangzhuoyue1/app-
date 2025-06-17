#include "../include/LogicSystem.h"
#include "../include/HttpConnection.h"
#include <spdlog/spdlog.h>
#include <AppInfoFetcher.h>
#include "../include/const.h"


void runSetupScript(const std::string& proxy_ip, const std::string& proxy_port, int uid) 
{
    std::string command = "./script.sh -u " + proxy_ip + " -p " + proxy_port + " -a " + std::to_string(uid);
    int ret = std::system(command.c_str());

    if (ret == 0) 
    {
        spdlog::info("流量转发配置脚本执行成功");
    } 
    else 
    {
        spdlog::error("流量转发配置脚本执行失败，返回码: {}", ret);
    }
}

void runClearScript() 
{
    std::string command = "./script.sh --cleanup";
    int ret = std::system(command.c_str());

    if (ret == 0) 
    {
        spdlog::info("流量转发清理脚本执行成功");
    } 
    else 
    {
        spdlog::error("流量转发清理脚本执行失败，返回码: {}", ret);
    }
}

void getAppInfo() 
{
    AppInfoFetCher fetcher;
    fetcher.fetchAllPackages();
    std::map<std::string, int> pkgUidMap = fetcher.getAllPackageUidMap();
    
    MySQLDAO dao;
    for (const auto& [pkg, uid] : pkgUidMap) {
        int result = dao.store_app_info_if_not_exists(uid, pkg);
        if (result == 1) {
            spdlog::info("Inserted new app: {} -> {}", pkg, uid);
        } else if (result == 0) {
            spdlog::info("App already exists: {} -> {}", pkg, uid);
        } else {
            spdlog::warn("Failed to insert app info for: {} -> {}", pkg, uid);
        }
    }
}

LogicSystem::LogicSystem() 
:m_traffic_capture( new TrafficCapture("com.test.app", "192.168.98.17")),
    m_zmq_subscriber(new ZMQSubscriber("tcp://0.0.0.0:5555"))
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
            root["role"] = user_info.type;
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

reg_post("/start_capture", [this](std::shared_ptr<HttpConnection> connection) {
    // 获取请求体
    auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
    spdlog::info("start_capture: Received body: {}", body_str);
    // 解析JSON
    json src_root;  
    src_root = json::parse(body_str);
    auto name = src_root["app_name"].get<std::string>();
    auto uid = src_root["app_uid"].get<int>();
    spdlog::info("start_capture: app_name: {}, app_uid: {}", name, uid);

    // 执行脚本
    runClearScript();
    runSetupScript("192.168.98.17","8080",uid);
    // 设置响应头
    connection->m_response.set(http::field::content_type, "application/json");
    //int uid=10051;
    //开始捕获
    m_traffic_capture->start_capture(uid);
    m_zmq_subscriber->start(uid);

    json root;
    root["error"] = 0;
    root["msg"] = "开始捕获数据包";
    // 发送响应
    std::string jsonstr = root.dump();
    beast::ostream(connection->m_response.body()) << jsonstr;
    spdlog::info("start_capture: response: {}", jsonstr);

    return true;

});

reg_post("/stop_capture", [this](std::shared_ptr<HttpConnection> connection) {
    // 获取请求体
    auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
    spdlog::info("stop_capture: Received body: {}", body_str);

    // 设置响应头
    connection->m_response.set(http::field::content_type, "application/json");

    //开始捕获
    m_traffic_capture->stop_capture();
    m_zmq_subscriber->stop();
    
    json root;
    root["error"] = 0;
    root["msg"] = "停止捕获数据包";
    // 发送响应
    std::string jsonstr = root.dump();
    beast::ostream(connection->m_response.body()) << jsonstr;
    spdlog::info("stop_capture: response: {}", jsonstr);

    return true;

});

reg_post("/get_app_info", [this](std::shared_ptr<HttpConnection> connection) {
    // 获取请求体
    auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
    spdlog::info("start_capture: Received body: {}", body_str);

    // 设置响应头
    connection->m_response.set(http::field::content_type, "application/json");

    getAppInfo();

    json root;
    root["error"] = 0;
    root["msg"] = "获取app信息成功"; 
    // 发送响应
    std::string jsonstr = root.dump();
    beast::ostream(connection->m_response.body()) << jsonstr;
    spdlog::info("start_capture: response: {}", jsonstr);

    return true;

});

reg_post("/user_mgr", [this](std::shared_ptr<HttpConnection> connection) {
    // 获取请求体
    auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
    spdlog::info("user_mgr: Received body: {}", body_str);
    // 解析JSON
    json src_root;  
    src_root = json::parse(body_str);
    auto username = src_root["username"].get<std::string>();
    auto req_id = src_root["req_id"].get<int>();
    spdlog::info("user_mgr: username: {}, req_id: {}", username, req_id);
    json root;

    switch (req_id)
    {
        case 1005://删除用户
        {
            m_mysql.del_user_by_name(username);
            spdlog::info("user_mgr: delete user: {}", username);
           
            root["error"] = 0;
            root["msg"] = "ok";
            break;
        }
        case 1006://添加用户
        {
            auto email = src_root["email"].get<std::string>();
            auto pwd = src_root["password"].get<std::string>();
            auto type = src_root["role"].get<std::string>();
            if(m_mysql.add_user_with_role(username, email, pwd, type)!=1)
            {
                spdlog::error("user_mgr: add user {} failed", username);
                root["error"] = 1;
                root["msg"] = "failed";
            }
            else
            {
                spdlog::info("user_mgr: add user {} success", username);
                root["error"] = 0;
                root["msg"] = "ok";
            }
            break;
        }
        case 1007://修改用户
        {
            UserInfo user_info;
            user_info.name = username;
            user_info.pwd = src_root["password"].get<std::string>();
            user_info.email = src_root["email"].get<std::string>();
            user_info.type = src_root["role"].get<std::string>();
            user_info.create_time = src_root["creat_time"].get<std::string>();
            user_info.last_login_time = src_root["login_time"].get<std::string>();
            m_mysql.update_user(user_info);
            spdlog::info("user_mgr: update user: {}", username);
            break;
        }
    }

    // 设置响应头
    connection->m_response.set(http::field::content_type, "application/json");

    

    // 发送响应
    std::string jsonstr = root.dump();
    beast::ostream(connection->m_response.body()) << jsonstr;
    spdlog::info("start_capture: response: {}", jsonstr);

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

