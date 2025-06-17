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

enum ReqId{
    ID_LOGIN_USER = 1001, //登录
    ID_REG_USER = 1002, //注册用户
    ID_START=1003,
    ID_STOP=1004,
    ID_DELETE_USER=1005,
    ID_ADD_USER=1006,
    ID_MODIFY_USER=1007,
};