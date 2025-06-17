#pragma once

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include <MySQLDAO.h>
#include "format.h"
#include <unordered_map>


/// @brief 订阅 mitmproxy 发布的所有 HTTP/HTTPS 报文 JSON，入队并异步存储到 MySQL
class ZMQSubscriber {
public:
    ZMQSubscriber(const std::string& address = "tcp://0.0.0.0:5555");
    ~ZMQSubscriber();

    /// @brief 启动订阅线程 & 存储线程
    void start(int uid = 10001);
    /// @brief 停止所有线程
    void stop();

private:
    void worker_thread();         // 负责接收 ZMQ 消息并入队
    void storage_thread();        // 负责从队列取出 JSON 并处理
    void db_worker_thread();      // 数据库工作线程
    void flush_cache();           // 将缓存写入数据库
    void parse_and_cache_http(const nlohmann::json& j);  // 报文解析并缓存
    void process_message(const nlohmann::json& j);

    zmq::context_t m_ctx;
    zmq::socket_t m_socket;
    std::thread m_worker;
    std::thread m_storer;
    std::vector<std::thread> m_dbWorkers;  // 数据库工作线程池
    std::atomic<bool> m_running;
    std::atomic<bool> m_dbRunning;

    std::queue<nlohmann::json> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    MySQLDAO m_dao;  // DAO 对象，用于存储

    std::unordered_map<std::string, HttpFlowInfo> m_flow_info_map; 
    std::vector<HttpPacket> m_packet_cache;
    std::mutex m_flow_mutex;  // 用于保护 flow_info_map
    std::mutex m_packet_mutex;  // 用于保护 packet_cache
    std::mutex m_dbMutex;     // 用于保护数据库任务队列
    std::condition_variable m_dbCv;
    std::queue<std::function<void()>> m_dbTasks;  // 数据库任务队列
    
    int app_uid;  // 模拟 app_uid
    
    // 统计信息
    std::atomic<size_t> m_totalPacketsReceived;
    std::atomic<size_t> m_totalPacketsStored;
};