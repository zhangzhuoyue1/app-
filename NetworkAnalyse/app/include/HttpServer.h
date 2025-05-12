#pragma once
#include "const.h"

//HTTP server
//用于处理http请求的类
class HttpServer :public std::enable_shared_from_this<HttpServer>
{
public:
    HttpServer(boost::asio::io_context& ioc,  unsigned short& port);

    void start();

private:
    tcp::acceptor                       m_acceptor;     //用于接受连接
    net::io_context&                    m_ioc;          //io_context对象，用于异步操作
    boost::asio::ip::tcp::socket        m_socket;       //socket对象，用于与客户端进行通信
};