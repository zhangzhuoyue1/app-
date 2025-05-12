#include "../include/HttpConnection.h"
#include "../include/HttpServer.h"
#include <spdlog/spdlog.h>

HttpServer::HttpServer(boost::asio::io_context& ioc, unsigned short& port)
    : m_acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    , m_ioc(ioc)
    , m_socket(ioc)
{
}

// 启动服务器，开始接受连接
void HttpServer::start()
{
    spdlog::info("HttpServer::start: Listening on port {}", m_acceptor.local_endpoint().port());
    auto self = shared_from_this();
    m_acceptor.async_accept(m_socket, [self](beast::error_code ec)
     {  
        try 
        {
            //出错则放弃这个连接，继续监听新链接
            if (ec) 
            {
                self->start();
                return;
            }
            //处理新链接，创建HpptConnection类管理新连接
            std::make_shared<HttpConnection>(std::move(self->m_socket))->start();
            //继续监听
            self->start();
        }
        catch (std::exception& exp) 
        {
            // 处理异常
            spdlog::error("Error in HttpServer::Start: {}", exp.what());
            self->start();
        }
    });
}