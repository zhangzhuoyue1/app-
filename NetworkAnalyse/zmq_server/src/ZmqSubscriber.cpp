// ZmqSubscriber.cpp
#include "ZmqSubscriber.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <ctime>

const int PACKET_THRESHOLD = 10;       // 报文数量阈值
const int TIME_THRESHOLD_SEC = 5;       // 时间间隔阈值（秒）
const size_t MAX_CACHE_SIZE = 1000;     // 最大缓存大小
const size_t DB_WORKER_THREADS = 2;     // 数据库工作线程数量
const int MAX_DB_RETRIES = 3;           // 数据库操作最大重试次数

// 辅助函数声明：将ISO时间转换为MySQL datetime格式
// 定义已移至 MySQLDAO.cpp，避免多重定义
std::string format_mysql_datetime(const std::string& iso_time);

ZMQSubscriber::ZMQSubscriber(const std::string& address)
  : m_ctx(1),
    m_socket(m_ctx, zmq::socket_type::sub),
    m_running(false),
    m_dbRunning(false),
    m_totalPacketsReceived(0),
    m_totalPacketsStored(0)
{
    m_socket.connect(address);
    m_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // 订阅全部消息
    spdlog::info("ZMQSubscriber initialized, connecting to {}", address);
}

ZMQSubscriber::~ZMQSubscriber() {
    stop();
}

void ZMQSubscriber::start(int uid) 
{
    app_uid = uid;
    spdlog::info("ZMQSubscriber started with app_uid={}", app_uid);
    if (!m_running.exchange(true)) 
    {
        // 启动接收线程
        m_worker = std::thread(&ZMQSubscriber::worker_thread, this);
        // 启动存储线程
        m_storer = std::thread(&ZMQSubscriber::storage_thread, this);
        
        // 启动数据库工作线程池
        m_dbRunning = true;
        for (size_t i = 0; i < DB_WORKER_THREADS; ++i) {
            m_dbWorkers.emplace_back(&ZMQSubscriber::db_worker_thread, this);
        }
        
        spdlog::info("ZMQSubscriber threads started: worker, storer, {} db workers", DB_WORKER_THREADS);
    }
}

void ZMQSubscriber::stop() {
    if (m_running.exchange(false)) {
        spdlog::info("ZMQSubscriber stopping...");
        
        // 唤醒可能在等待的存储线程
        m_cv.notify_all();
        
        // 等待工作线程和存储线程结束
        if (m_worker.joinable())
            m_worker.join();
        if (m_storer.joinable())
            m_storer.join();
            
        // 停止数据库工作线程
        {
            std::lock_guard<std::mutex> lk(m_dbMutex);
            m_dbRunning = false;
        }
        m_dbCv.notify_all();
        for (auto& thread : m_dbWorkers) {
            if (thread.joinable())
                thread.join();
        }
        
        // 清空剩余缓存
        flush_cache();
        
        // 关闭ZMQ连接
        m_socket.close();
        
        spdlog::info("ZMQSubscriber stopped. Received {} packets, stored {} packets", 
                    m_totalPacketsReceived.load(), m_totalPacketsStored.load());
    }
}

void ZMQSubscriber::worker_thread() {
    spdlog::info("Worker thread started");
    while (m_running) {
        zmq::message_t msg;
        if (m_socket.recv(msg, zmq::recv_flags::dontwait)) {
            try {
                auto j = nlohmann::json::parse(msg.to_string());
                m_totalPacketsReceived++;
                process_message(j);
            } catch (const std::exception& e) {
                spdlog::error("JSON parse error: {}", e.what());
            }
        } else {
            // 短暂休眠避免CPU占用过高
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    spdlog::info("Worker thread stopped");
}

void ZMQSubscriber::process_message(const nlohmann::json& j) {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push(j);
    }
    m_cv.notify_one();
}

void ZMQSubscriber::storage_thread() {
    spdlog::info("Storage thread started");
    auto last_flush_time = std::chrono::steady_clock::now();
    
    while (m_running) {
        nlohmann::json j;
        bool has_data = false;
        
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            // 等待数据或超时
            has_data = m_cv.wait_for(lk, std::chrono::seconds(1), [&] {
                return !m_running.load() || !m_queue.empty();
            });
            
            if (!m_running && m_queue.empty())
                break;
                
            if (has_data) {
                j = std::move(m_queue.front());
                m_queue.pop();
            }
        }

        // 处理接收到的数据
        if (has_data) {
            j["app_uid"] = app_uid;
            
            try {
                parse_and_cache_http(j);
            } catch (const std::exception& e) {
                spdlog::error("Error processing packet: {}", e.what());
            }
        }

        // 检查是否需要定时刷新
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_flush_time).count() >= TIME_THRESHOLD_SEC) {
            flush_cache();
            last_flush_time = now;
        }
    }
    
    // 停止前确保刷入剩余数据
    flush_cache();
    spdlog::info("Storage thread stopped");
}

void ZMQSubscriber::parse_and_cache_http(const nlohmann::json& j) {
    // 检查消息类型
    std::string msg_type = j.value("type", "");
    
    // 统计信息
    static std::atomic<size_t> flow_info_count(0);
    static std::atomic<size_t> request_count(0);
    static std::atomic<size_t> response_count(0);
    
    if (msg_type == "flow_info") {
        // 处理flow_info类型的消息
        flow_info_count++;
        
        HttpFlowInfo flow;
        flow.flow_id = j.value("flow_id", "");
        if (flow.flow_id.empty()) {
            spdlog::error("flow_info missing 'flow_id', skipping");
            return;
        }
        
        flow.app_uid = j.value("app_uid", 0);
        flow.protocol = j.value("protocol", "TCP");
        flow.top_protocol = j.value("top_protocol", "HTTP");
        flow.src_ip = j.value("src_ip", "");
        flow.src_port = j.value("src_port", 0);
        flow.dst_ip = j.value("dst_ip", "");
        flow.dst_port = j.value("dst_port", 0);
        flow.http_version = j.value("http_version", "1.1");
        flow.host = j.value("host", "");
        flow.url = j.value("url", "");
        flow.method = j.value("method", "");
        flow.status_code = j.value("status", 0);
        flow.content_type = j.value("content_type", "");
        flow.start_time = j.value("timestamp", "");
        
        // 验证时间戳格式
        if (!flow.start_time.empty()) {
            flow.start_time = format_mysql_datetime(flow.start_time);
        } else {
            spdlog::warn("flow_info has empty timestamp, using current time");
            flow.start_time = format_mysql_datetime("");
        }
        
        // 保存flow_info
        {
            std::lock_guard<std::mutex> lk(m_flow_mutex);
            m_flow_info_map[flow.flow_id] = flow;
            spdlog::debug("Added flow_info: {}", flow.flow_id);
        }
        
        // 每处理1000个flow_info打印统计信息
        if (flow_info_count % 1000 == 0) {
            spdlog::info("Processed {} flow_info messages", flow_info_count.load());
        }
    } else {
        // 处理普通HTTP包 (request/response)
        HttpPacket pkt;
        pkt.flow_id = j.value("flow_id", "");
        if (pkt.flow_id.empty()) {
            spdlog::error("Packet missing 'flow_id', skipping");
            return;
        }
        
        // 验证direction字段
        std::string direction = j.value("direction", "");
        if (direction != "request" && direction != "response") {
            spdlog::warn("Invalid direction '{}' in packet, flow_id={}", direction, pkt.flow_id);
            spdlog::debug("Full packet content: {}", j.dump());
            return; // 跳过无效包
        }
        pkt.type = direction;
        
        // 记录请求/响应计数
        if (direction == "request") {
            request_count++;
        } else {
            response_count++;
        }
        
        // 解析其他字段
        pkt.headers = j.value("headers", nlohmann::json{});
        pkt.body = j.value("info", "");
        pkt.timestamp = j.value("timestamp", "");
        pkt.top_protocol = j.value("top_protocol", "HTTP");
        pkt.content_type = j.value("content_type", "");
        pkt.length = j.value("length", 0);
        
        // 验证并格式化时间戳
        if (!pkt.timestamp.empty()) {
            // 保存原始时间戳
            std::string original_ts = pkt.timestamp;
            // 转换为MySQL格式
            pkt.timestamp = format_mysql_datetime(pkt.timestamp);
            
            // 记录时间戳转换前后的对比（调试用）
            if (original_ts != pkt.timestamp) {
                spdlog::trace("Timestamp converted from '{}' to '{}'", original_ts, pkt.timestamp);
            }
        } else {
            spdlog::warn("Packet has empty timestamp, using current time");
            pkt.timestamp = format_mysql_datetime("");
        }
        
        spdlog::trace("Processing packet: flow_id={}, type={}, top_protocol={}",
                 pkt.flow_id, pkt.type, pkt.top_protocol);
        
        // 处理flow_info_map（更新请求/响应信息）
        {
            std::lock_guard<std::mutex> lk(m_flow_mutex);

            auto it = m_flow_info_map.find(pkt.flow_id);
            if (it == m_flow_info_map.end()) 
            {
                // 如果flow_info不存在，创建一个基础版本
                // 注意：这可能发生在response先于request到达的情况下
                // 通常不应该发生，因为mitmproxy先处理request
                spdlog::warn("flow_info not found for packet, creating basic version: {}", pkt.flow_id);
                
                HttpFlowInfo flow;
                flow.flow_id = pkt.flow_id;
                flow.app_uid = j.value("app_uid", 0);
                flow.protocol = j.value("protocol", "TCP");
                flow.top_protocol = pkt.top_protocol;
                flow.src_ip = j.value("src_ip", "");
                flow.src_port = j.value("src_port", 0);
                flow.dst_ip = j.value("dst_ip", "");
                flow.dst_port = j.value("dst_port", 0);
                flow.http_version = j.value("http_version", "1.1");
                flow.host = j.value("host", "");
                flow.url = j.value("url", "");
                flow.start_time = pkt.timestamp;
                
                // 根据packet类型设置method或status_code
                if (pkt.type == "request") {
                    flow.method = j.value("method", "");
                    flow.status_code = 0;
                } else {
                    flow.method = "";
                    flow.status_code = j.value("status_code", 0);
                }
                
                flow.content_type = pkt.content_type;
                
                m_flow_info_map[pkt.flow_id] = flow;
                spdlog::debug("Created basic flow_info for packet: {}", pkt.flow_id);
            } 
            else 
            {
                // 更新 existing flow_id
                HttpFlowInfo& flow = it->second;
                if (pkt.type == "response") {
                    flow.status_code = j.value("status_code", flow.status_code);
                    flow.content_type = j.value("content_type", flow.content_type);
                    spdlog::trace("Updated response for flow_id: {}", pkt.flow_id);
                }
                else
                {
                    flow.method = j.value("method", flow.method);
                    //flow.host = j.value("host", flow.host);
                    flow.url = j.value("url", flow.url);
                    spdlog::trace("Updated request for flow_id: {}", pkt.flow_id);
                }
            }
        }
        
        // 处理packet缓存
        bool shouldFlush = false;
        {
            std::lock_guard<std::mutex> lk(m_packet_mutex);
            
            // 检查是否超过最大缓存限制
            if (m_packet_cache.size() >= MAX_CACHE_SIZE) {
                spdlog::warn("Packet cache reached maximum size ({}), flushing immediately", MAX_CACHE_SIZE);
                shouldFlush = true;
            } else {
                m_packet_cache.push_back(pkt);
                spdlog::info("m_packet_cache size: {}", m_packet_cache.size());
                // 检查是否达到数量阈值
                if (m_packet_cache.size() >= PACKET_THRESHOLD) {
                    shouldFlush = true;
                }
            }
        }
        
        // 每处理1000个packet打印统计信息
        if ((request_count + response_count) % 1000 == 0) {
            spdlog::info("Processed {} requests, {} responses", 
                        request_count.load(), response_count.load());
        }
        
        // 如果需要刷新缓存，在锁外部调用flush_cache
        if (shouldFlush) {
            flush_cache();
        }
    }
}

void ZMQSubscriber::flush_cache() {
    std::vector<HttpFlowInfo> flow_copy;
    std::vector<HttpPacket> pkt_copy;

    // 复制缓存数据
    {
        std::lock_guard<std::mutex> lk(m_flow_mutex);
        flow_copy.reserve(m_flow_info_map.size());
        for (const auto& [flow_id, flow_info] : m_flow_info_map) {
            flow_copy.push_back(flow_info);
        }
        m_flow_info_map.clear();
    }

    {
        std::lock_guard<std::mutex> lk(m_packet_mutex);
        std::swap(pkt_copy, m_packet_cache);
    }

    if (!flow_copy.empty() || !pkt_copy.empty()) {
        // 将数据库操作放入任务队列
        {
            std::lock_guard<std::mutex> lk(m_dbMutex);
            m_dbTasks.push([this, flow_copy, pkt_copy]() {
                size_t flows_inserted = 0;
                size_t packets_inserted = 0;
                
                // 插入 flow_info
                for (const auto& flow : flow_copy) {
                    int retries = 0;
                    bool success = false;
                    
                    while (retries < MAX_DB_RETRIES && !success) {
                        if (m_dao.insert_http_flow_info(flow)) {
                            success = true;
                            spdlog::info("Inserted http_flow_info: {}", flow.flow_id);
                            flows_inserted++;
                        } else {
                            retries++;
                            spdlog::error("Failed to insert http_flow_info (attempt {}): {}", 
                                        retries, flow.flow_id);
                            std::this_thread::sleep_for(std::chrono::milliseconds(100 * retries));
                        }
                    }
                    
                    if (!success) {
                        spdlog::error("Giving up on http_flow_info after {} retries: {}", 
                                    MAX_DB_RETRIES, flow.flow_id);
                    }
                }

                // 插入 packet
                for (const auto& pkt : pkt_copy) {
                    int retries = 0;
                    bool success = false;
                    
                    while (retries < MAX_DB_RETRIES && !success) {
                        if (m_dao.insert_http_packet(pkt)) {
                            success = true;
                            spdlog::info("Inserted http_packet: {}", pkt.flow_id);
                            packets_inserted++;
                        } else {
                            retries++;
                            spdlog::error("Failed to insert http_packet (attempt {}): {}", 
                                        retries, pkt.flow_id);
                            std::this_thread::sleep_for(std::chrono::milliseconds(100 * retries));
                        }
                    }
                    
                    if (!success) {
                        spdlog::error("Giving up on http_packet after {} retries: {}", 
                                    MAX_DB_RETRIES, pkt.flow_id);
                    }
                }

                m_totalPacketsStored += packets_inserted;
                spdlog::info("Flushed to database: {} flow_infos, {} packets", 
                             flows_inserted, packets_inserted);
            });
        }
        m_dbCv.notify_one();
    }
}

void ZMQSubscriber::db_worker_thread() {
    spdlog::info("DB worker thread started");
    
    while (m_dbRunning) {
        std::function<void()> task;
        bool has_task = false;
        
        {
            std::unique_lock<std::mutex> lk(m_dbMutex);
            has_task = m_dbCv.wait_for(lk, std::chrono::seconds(1), [&] {
                return !m_dbRunning || !m_dbTasks.empty();
            });
            
            if (has_task && !m_dbTasks.empty()) {
                task = std::move(m_dbTasks.front());
                m_dbTasks.pop();
            }
        }
        
        if (has_task) {
            try {
                task();
            } catch (const std::exception& e) {
                spdlog::error("Exception in DB worker thread: {}", e.what());
            }
        }
    }
    
    spdlog::info("DB worker thread stopped");
}