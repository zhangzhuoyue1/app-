#pragma once
#include <queue>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include <MySQLDAO.h>
#include "format.h"

//extern   std::map<std::string, std::queue<nlohmann::json>>    g_parsed_packets_map; // 存储解析后的 JSON 数据

struct Packet {
    timeval timestamp; // 时间戳
    std::vector<uint8_t> data; // 数据
};

class PacketParser {
public:
    PacketParser();
    ~PacketParser();

    void                push_raw_packet(const Packet& packet); // 添加原始数据包
    void                start(int uid=10001);
    void                stop();
    //bool                get_parsed_packet(const std::string& protocol, nlohmann::json& result); // 获取解析结果队列

private:
    void                parse_loop();  // 解析线程主循环
    void                parse_packet(const Packet& packet);   // 解析数据包
    void                start_storage();

    // 协议解析器
    nlohmann::json      parse_tcp(const uint8_t* data, size_t len, const timeval& ts,
                             const std::string& src_ip, const std::string& des_ip,
                             const uint8_t* ip_header_ptr, size_t ip_header_lenp); // 解析 TCP 数据包
    nlohmann::json      parse_udp(const uint8_t* data, size_t len, const timeval& ts,
                              const std::string& src_ip, const std::string& des_ip,
                            const uint8_t* ip_header_ptr, size_t ip_header_len);
    nlohmann::json      parse_dns(const uint8_t* data, size_t len, const timeval& ts,
                            const std::string& src_ip, const std::string& des_ip,
                            const uint8_t* ip_header_ptr, size_t ip_header_len);

    std::string         parse_tcp_flags(uint8_t flags);     // 解析 TCP 标志

    std::string         format_timeval(const timeval& tv);          //转换时间戳格式

    //会话
    void                session_management_loop();
    
    void                flush_pending_sessions();

    std::atomic<bool>                                   m_running;
    std::thread                                         m_parser_thread;
    std::queue<Packet>                                  m_raw_packet_queue;     // 原始数据包队列
    std::mutex                                          m_raw_mutex;
    std::condition_variable                             m_raw_cv;

    std::map<std::string, std::mutex>                   m_parsed_mutex;

    MySQLDAO                                            m_mysql;          // 数据库对象

    std::thread                                         m_storage_thread;   // 存储线程
    std::queue<json>                                    m_storage_queue;
    std::mutex                                          m_storage_mutex;
    std::condition_variable                             m_storage_cv;

    //     // 会话缓存（五元组哈希 -> 会话信息）
    // std::unordered_map<std::string, SessionInfo>        m_sessionCache;
    // std::mutex                                          m_sessionMutex;
    
    // // 待更新的会话队列
    std::queue<SessionInfo>                             m_pendingSessions;
    std::mutex                                          m_pendingMutex;
    std::condition_variable                             m_pendingCV;
    
    // // 会话管理线程
    std::thread                                         m_sessionThread;
    // int                                                 m_flushInterval; // 刷新间隔（秒）
    // int                                                 m_sessionTimeout; // 会话超时时间（秒）
    // const size_t                                        MAX_SESSIONS_PER_FLUSH = 100;   
    
    // 新增存储控制参数
    const size_t MIN_SESSIONS_BEFORE_FLUSH = 20;  // 最小存储会话数
    const int FLUSH_TIMEOUT_SECONDS = 5;         // 存储超时时间(秒)
    
    std::map<std::string, SessionInfo>           m_activeSessions; // 活跃会话集
    std::mutex                                   m_sessionsMutex;
    std::time_t                                  m_lastFlushTime;  // 上次存储时间
    std::atomic<bool>                            m_flushInProgress; // 存储进行中标志

    std::string             m_src_ip="192.168.31.200";
    int                     app_uid=10001;
};
