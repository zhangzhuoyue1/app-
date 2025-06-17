#include <spdlog/spdlog.h>
#include <iostream>
#include "../include/HttpServer.h"

int main()
{
try
    {
        unsigned short port = static_cast<unsigned short>(7777);
        net::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) 
        {
            if (error) {
                return;
            }
            ioc.stop();
            });
        std::make_shared<HttpServer>(ioc, port)->start();
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}