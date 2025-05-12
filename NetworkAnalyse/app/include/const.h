#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//char 转为16进制
unsigned char ToHex(unsigned char x);


//16进制转为char
unsigned char FromHex(unsigned char x);


//url编码
std::string UrlEncode(const std::string& str);

//url解码
std::string UrlDecode(const std::string& str);

enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,  //Json解析错误
    RPCFailed = 1002,  //RPC请求错误
};