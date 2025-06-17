#include <string>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;   

// struct SessionInfo {
//     int app_uid;        // 应用唯一 ID
//     std::string timestamp;
//     std::string session_id;      // 基于五元组的唯一 ID
//     std::string protocol;        // TCP/UDP/HTTP
//     std::string src_ip;
//     int src_port;
//     std::string dst_ip;
//     int dst_port;
//     std::vector<std::string> accessed_resources;  // host+path
//     std::vector<json> packets;   // 所有相关的 JSON 报文
// };

// struct HttpFlowInfo {
//     std::string flow_id;
//     int app_uid;
//     std::string protocol;
//     std::string top_protocol;
//     std::string src_ip;
//     std::string http_version; // HTTP/1.1, HTTP/2, etc.
//     int src_port;
//     std::string dst_ip;
//     int dst_port;
//     std::string host;
//     std::string url;
//     std::string method;
//     int status_code;
//     std::string content_type;
//     std::string start_time;
// };

// struct HttpPacket {
//     std::string flow_id;
//     std::string type; // request / response
//     std::string top_protocol;
//     nlohmann::json headers;
//     std::string body;
//     std::string timestamp;
// };
