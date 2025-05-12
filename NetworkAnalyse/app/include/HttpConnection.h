#include "const.h"


class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem;
public:
    HttpConnection(tcp::socket socket);
    void                start();                // 启动连接
private:
    void                check_deadline();       // 检查超时  
    void                write_response();       // 写入响应
    void                handle_req();           // 处理请求
    void                preparse_get_param();   // 预解析GET请求参数              

    tcp::socket                             m_socket;            // socket对象
    beast::flat_buffer                      m_buffer{ 8192 };    // 缓冲区
    http::request<http::dynamic_body>       m_request;           // http请求对象
    http::response<http::dynamic_body>      m_response;          // http响应对象
    std::string                             m_get_url;           // GET请求的url
    std::unordered_map<std::string, std::string> m_get_params;   // GET请求的参数

    net::steady_timer m_deadline{
        m_socket.get_executor(), std::chrono::seconds(60) };     // 超时定时器
};