#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"
#include <MySQLDAO.h>
#include <TrafficCapture.h>
#include <ZmqSubscriber.h>

using json = nlohmann::json;
class HttpConnection;

// HttpHandler是一个函数指针类型，用于处理http请求
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;   

// LogicSystem类用于处理http请求
class LogicSystem :public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    bool                handle_get(std::string, std::shared_ptr<HttpConnection>);   // 处理GET请求
    void                reg_get(std::string, HttpHandler handler);                  // 注册GET请求处理器
    void                reg_post(std::string, HttpHandler handler);                  // 注册POST请求处理器
    bool                handle_post(std::string, std::shared_ptr<HttpConnection>);  // 处理POST请求
private:
    LogicSystem();
    std::map<std::string, HttpHandler>              m_post_handlers;  // POST请求处理器
    std::map<std::string, HttpHandler>              m_get_handlers;   // GET请求处理器
    MySQLDAO                                        m_mysql;          // 数据库对象
    TrafficCapture*                                 m_traffic_capture; // 流量捕获对象
    ZMQSubscriber*                                  m_zmq_subscriber;  // ZMQ订阅对象                         
};