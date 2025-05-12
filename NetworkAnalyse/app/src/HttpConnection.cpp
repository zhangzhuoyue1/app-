#include "../include/HttpConnection.h"
#include "../include/LogicSystem.h"
#include <spdlog/spdlog.h>

HttpConnection::HttpConnection(tcp::socket socket)
    : m_socket(std::move(socket)) 
{
}

void HttpConnection::start() 
{
    auto self = shared_from_this();
    http::async_read(m_socket, m_buffer, m_request, [self](beast::error_code ec,
        std::size_t bytes_transferred) {
            try {
                if (ec) {
                    // 处理错误
                    spdlog::error("Error in HttpConnection::start: {}", ec.message());
                    return;
                }
                spdlog::info("HttpConnection::start: read {} bytes", bytes_transferred);

                //处理读到的数据
                boost::ignore_unused(bytes_transferred);
                self->handle_req();
                self->check_deadline();
            }
            catch (std::exception& exp) {
                // 处理异常
                spdlog::error("Error in HttpConnection::start: {}", exp.what());
            }
        }
    );
}



void HttpConnection::handle_req()
{
    //设置版本
    m_response.version(m_request.version());
    //设置为短链接
    m_response.keep_alive(false);
    
    //判断是否为Get请求
    if (m_request.method() == http::verb::get) {
        preparse_get_param();
        bool success = LogicSystem::get_instance()->handle_get(m_get_url, shared_from_this());
        spdlog::info("HttpConnection::handle_req: get request, url: {}", std::string(m_request.target()));

        //处理GET请求
        if (!success) {
            m_response.result(http::status::not_found);
            m_response.set(http::field::content_type, "text/plain");
            beast::ostream(m_response.body()) << "url not found\r\n";
            write_response();   
            return;
        }

        m_response.result(http::status::ok);                // 设置响应状态码
        m_response.set(http::field::server, "HttpServer");  // 设置服务器名称
        write_response();
        return;
    }

    if (m_request.method() == http::verb::post) {
        bool success = LogicSystem::get_instance()->handle_post(std::string(m_request.target()), shared_from_this());
        spdlog::info("HttpConnection::handle_req: post request, url: {}", std::string(m_request.target()));
        if (!success) {
            m_response.result(http::status::not_found);
            m_response.set(http::field::content_type, "text/plain");
            beast::ostream(m_response.body()) << "url not found\r\n";
            write_response();
            return;
        }
        m_response.result(http::status::ok);
        m_response.set(http::field::server, "HttpServer");
        write_response();
        return;
    }
} 

//返回响应
void HttpConnection::write_response() 
{
    auto self = shared_from_this();
    m_response.content_length(m_response.body().size());
    http::async_write(
        m_socket,
        m_response,
        [self](beast::error_code ec, std::size_t)
        {
            self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->m_deadline.cancel();
        });
}

void HttpConnection::check_deadline() 
{
    auto self = shared_from_this();
    // 设置超时
    m_deadline.async_wait(
        [self](beast::error_code ec)
        {
            if (!ec)
            {
                // Close socket to cancel any outstanding operation.
                self->m_socket.close(ec);
            }
        });
}

void HttpConnection::preparse_get_param() 
{
    // 提取 URI  
    auto uri = m_request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）  
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) 
    {
        m_get_url = std::string(uri);
        return;
    }
    // 提取查询字符串
    m_get_url = std::string(uri.substr(0, query_pos));
    std::string query_string = std::string(uri.substr(query_pos + 1));
    std::string key;
    std::string value;
    size_t pos = 0;
    // 解析查询字符串
    while ((pos = query_string.find('&')) != std::string::npos) 
    {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) 
        {
            key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
            value = UrlDecode(pair.substr(eq_pos + 1));
            m_get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）  
    if (!query_string.empty()) 
    {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) 
        {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            m_get_params[key] = value;
        }
    }
}
