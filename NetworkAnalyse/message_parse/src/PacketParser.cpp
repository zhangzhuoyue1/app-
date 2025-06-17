#include "PacketParser.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <arpa/inet.h>
#include <spdlog/spdlog.h>

using json = nlohmann::json;
std::map<std::string, std::queue<nlohmann::json>>    g_parsed_packets_map; // 存储解析后的 JSON 数据

// 生成会话ID（五元组哈希）
std::string generateSessionId(const std::string& src_ip, int src_port, 
                                 const std::string& dst_ip, int dst_port,
                                 const std::string& protocol) {
        // 简单的会话ID生成方式，可根据需要优化
    return src_ip + ":" + std::to_string(src_port) + "-" + 
            dst_ip + ":" + std::to_string(dst_port) + "-" + protocol;
}

PacketParser::PacketParser() : m_running(false) 
{

}

PacketParser::~PacketParser() 
{
    stop();
}

void PacketParser::start(int uid) 
{
    m_running = true;
    m_parser_thread = std::thread(&PacketParser::parse_loop, this);
    m_sessionThread = std::thread(&PacketParser::session_management_loop, this);
    spdlog::info("PacketParser started");
    start_storage();
    spdlog::info("PacketParser Storage started");
    app_uid=uid;
}

void PacketParser::stop() 
{
    m_running = false;
    // 通知等待中的线程
    m_raw_cv.notify_all();
    m_storage_cv.notify_all();  
    m_pendingCV.notify_all();
        
    if (m_parser_thread.joinable()) 
        m_parser_thread.join();
    if (m_storage_thread.joinable())
        m_storage_thread.join();
    if (m_sessionThread.joinable())
        m_sessionThread.join();
                
        // 刷新所有待处理的会话
    flush_pending_sessions();
}
    

    // 会话管理线程函数
void PacketParser::session_management_loop() {
    while (m_running) {
        spdlog::debug("Session management thread checking for flush condition");
        
        // 检查是否需要刷新
        bool shouldFlush = false;
        std::time_t now = std::time(nullptr);
        
        {
            std::lock_guard<std::mutex> sessionsLock(m_sessionsMutex);
            shouldFlush = (m_activeSessions.size() >= MIN_SESSIONS_BEFORE_FLUSH || 
                          now - m_lastFlushTime >= FLUSH_TIMEOUT_SECONDS);
        }
        
        if (shouldFlush) {
            flush_pending_sessions(); // 刷新所有会话
        } else {
            // 等待一段时间后再次检查
            std::unique_lock<std::mutex> lock(m_pendingMutex);
            m_pendingCV.wait_for(lock, std::chrono::seconds(1));
        }
    }
    
    // 程序停止前刷新所有会话
    flush_pending_sessions();
    spdlog::info("Session management thread stopped");
}


// 刷新待处理的会话到数据库
void PacketParser::flush_pending_sessions() {
        if (m_flushInProgress) return; // 避免并发刷新
    
    m_flushInProgress = true;
    std::queue<SessionInfo> sessionsToFlush;
    std::time_t now = std::time(nullptr);
    
    {
        std::lock_guard<std::mutex> sessionsLock(m_sessionsMutex);
        spdlog::info("Flushing all sessions: count={}, last flush={}s ago", 
                    m_activeSessions.size(), now - m_lastFlushTime);
        
        // 复制所有会话到待刷新队列
        for (const auto& pair : m_activeSessions) {
            sessionsToFlush.push(pair.second);
        }
        
        // 清空活跃会话集
        m_activeSessions.clear();
        m_lastFlushTime = now;
    }
    
    // 执行数据库写入
    while (!sessionsToFlush.empty()) {
        const auto& session = sessionsToFlush.front();
        if (m_mysql.insert_or_update_session_info(session) != 1) {
            spdlog::error("Failed to store session: {}", session.session_id);
        }
        sessionsToFlush.pop();
    }
    
    m_flushInProgress = false;
}

/*************  ✨ Windsurf Command ⭐  *************/
/// @brief Pushes a raw packet into the processing queue.
/// @param packet The packet to be added to the queue.

/*******  066d4dd9-735d-45c9-9f96-875126b33a4d  *******/
void PacketParser::push_raw_packet(const Packet& packet) 
{
    // 将原始数据包推入队列
    std::lock_guard<std::mutex> lock(m_raw_mutex);
    Packet raw_packet = packet;
    m_raw_packet_queue.push(raw_packet);
    //spdlog::info("Pushed raw packet to queue, size: {}", m_raw_packet_queue.size());
    m_raw_cv.notify_one();
}

void PacketParser::parse_loop() 
{
    while (m_running) 
    {
        Packet packet;
        {
            std::unique_lock<std::mutex> lock(m_raw_mutex);
            // 等待直到有数据包可用
            m_raw_cv.wait(lock, [&] { 
                return !m_raw_packet_queue.empty() || !m_running; });

            if (!m_running) break;  

            packet = m_raw_packet_queue.front();
            m_raw_packet_queue.pop();
        }

        // 解析数据包
        parse_packet(packet);
    }
}

void PacketParser::parse_packet(const Packet& packet) 
{
    if (packet.data.size() < sizeof(ETHER_HEADER)) return;

    const uint8_t* data = packet.data.data();
    ETHER_HEADER* eth = (ETHER_HEADER*)data;
    // std::ostringstream ss;
    // for (size_t i = 0; i < std::min(packet.data.size(), size_t(64)); ++i)
    //     ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";spdlog::info("Raw packet hex dump: {}", ss.str());
    // spdlog::info("Raw packet size: {}", packet.data.size());
    // spdlog::info("Ether type: {:#06x}", ntohs(eth->ether_type));
    auto time = packet.timestamp;

    json parsed;

    switch (ntohs(eth->ether_type)) 
    {
        case 0x0800: // IP
        {
            // spdlog::info("Parsing IP packet");
            if (packet.data.size() < sizeof(ETHER_HEADER) + sizeof(IP_HEADER)) return;
            const IP_HEADER* ip = reinterpret_cast<const IP_HEADER*>(data + sizeof(ETHER_HEADER));
                size_t ip_header_len = (ip->versiosn_head_length & 0x0F) * 4;

            if (packet.data.size() < sizeof(ETHER_HEADER) + ip_header_len) return;

            // 获取源 IP 和目的 IP 地址
            char src_ip[INET_ADDRSTRLEN] = {0};
            char des_ip[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &(ip->src_addr), src_ip, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(ip->des_addr), des_ip, INET_ADDRSTRLEN);

            const uint8_t* transport = data + sizeof(ETHER_HEADER) + ip_header_len;
            size_t transport_len = packet.data.size() - sizeof(ETHER_HEADER) - ip_header_len;
            switch (ip->protocol) 
            {
                case 6: // TCP

                    parsed = parse_tcp(transport, packet.data.size() - sizeof(ETHER_HEADER) - ip_header_len,time, src_ip, des_ip , 
                    data + sizeof(ETHER_HEADER), ip_header_len);
                    break;
                case 17: // UDP
                {
                    //spdlog::info("Parsing UDP packet");
                    const UDP_HEADER* udp = reinterpret_cast<const UDP_HEADER*>(transport);
                    uint16_t src_port = ntohs(udp->src_port);
                    uint16_t des_port = ntohs(udp->des_port);

                     if (src_port == 53 || des_port == 53) 
                    {
                        const uint8_t* udp_payload = transport + sizeof(UDP_HEADER);
                        size_t udp_payload_len = transport_len - sizeof(UDP_HEADER);

                        parsed = parse_dns(udp_payload, udp_payload_len, time, src_ip, des_ip,
                                 data + sizeof(ETHER_HEADER), ip_header_len);
                    }

                    // parsed = parse_udp(transport, transport_len, time, src_ip, des_ip,
                    //      data + sizeof(ETHER_HEADER), ip_header_len);
                    break;
                }
                default:
                    return;
            }
            break;
        }
        case 0x0806: // ARP
            break;
        default:
            return;
    }

    // 如果解析成功, 将结果添加到相应的队列
    if (!parsed.is_null()) 
    {
        parsed["app_uid"] = app_uid;
        //改为异步存储
        {
            std::lock_guard<std::mutex> lock(m_storage_mutex);
            m_storage_queue.push(parsed);
        }
        m_storage_cv.notify_one();
    }
}

// json PacketParser::parse_tcp(const uint8_t* data, size_t len, const timeval& ts,
//                              const std::string& src_ip, const std::string& des_ip,
//                              const uint8_t* ip_header_ptr, size_t ip_header_len)
// {
//     json j;
    
//     if(des_ip=="192.168.31.172")
//     {
//         j["src_ip"] = src_ip;
//         j["des_ip"] = m_src_ip;
//     }
//     else if(src_ip=="192.168.31.172")
//     {
        
//         j["src_ip"] = m_src_ip;
//         j["des_ip"] = des_ip;
//     }

//     if (len < sizeof(TCP_HEADER)) return j;
//     const TCP_HEADER* tcp = reinterpret_cast<const TCP_HEADER*>(data);

//     size_t tcp_header_len = (tcp->header_length >> 4) * 4;
//     if (len < tcp_header_len) return j;

//     const uint8_t* payload = data + tcp_header_len;
//     size_t payload_len = len - tcp_header_len;

//     // 基本字段封装
//     j["app_uid"] = app_uid;
//     j["protocol"] = "TCP";
//     j["timestamp"] = format_timeval(ts);
//     j["src_port"] = static_cast<int>(ntohs(tcp->src_port));
//     j["des_port"] = static_cast<int>(ntohs(tcp->des_port));
//     j["sequence"] = static_cast<uint64_t>(ntohl(tcp->sequence));
//     j["ack"] = static_cast<uint64_t>(ntohl(tcp->ack));
//     // j["flags"] = static_cast<uint8_t>(tcp->flags);
//     j["flags"] = parse_tcp_flags(tcp->flags);
//     j["packet_len"] = static_cast<int>(len + ip_header_len);

//     // 封装头部（十六进制字符串）
//     std::ostringstream header_ss;
//     for (size_t i = 0; i < ip_header_len + tcp_header_len; ++i)
//         header_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ip_header_ptr[i]);
//     j["header"] = header_ss.str();

//     // 封装 payload（十六进制字符串）
//     std::ostringstream payload_ss;
//     for (size_t i = 0; i < payload_len; ++i)
//         payload_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(payload[i]);
//     j["data"] = payload_ss.str();

//     // spdlog::info("Parsed TCP packet: timestamp={}, src_ip={}, des_ip={}, src_port={}, des_port={}, sequence={}, ack={},packet_len={}",
//     //             j["timestamp"].get<std::string>(),
//     //             j["src_ip"].get<std::string>(), j["des_ip"].get<std::string>(),
//     //             j["src_port"].get<int>(), j["des_port"].get<int>(),
//     //             j["sequence"].get<uint64_t>(), j["ack"].get<uint64_t>(),
//     //             j["packet_len"].get<int>());

//     return j;
// }

 // TCP解析函数（修改部分）
json PacketParser::parse_tcp(const uint8_t* data, size_t len, const timeval& ts,
                                const std::string& src_ip, const std::string& des_ip,
                                const uint8_t* ip_header_ptr, size_t ip_header_len)
{
   
   json j;    
    // 确定实际的源IP和目的IP
    std::string actualSrcIp, actualDesIp;
    if (des_ip == "192.168.31.172") {
        actualSrcIp = src_ip;
        actualDesIp = m_src_ip;
    } else if (src_ip == "192.168.31.172") {
        actualSrcIp = m_src_ip;
        actualDesIp = des_ip;
    } else {
        actualSrcIp = src_ip;
        actualDesIp = des_ip;
    }

    if (len < sizeof(TCP_HEADER)) return j;
    const TCP_HEADER* tcp = reinterpret_cast<const TCP_HEADER*>(data);
    size_t tcp_header_len = (tcp->header_length >> 4) * 4;
    if (len < tcp_header_len) return j;

    // 构建五元组
    std::string protocol = "TCP";
    int src_port = static_cast<int>(ntohs(tcp->src_port));
    int dst_port = static_cast<int>(ntohs(tcp->des_port));
    std::string sessionId = generateSessionId(actualSrcIp, src_port, actualDesIp, dst_port, protocol);

    std::unique_lock<std::mutex> sessionsLock(m_sessionsMutex);
    std::time_t now = std::time(nullptr);
    bool shouldFlush = false;

    // 更新或创建会话
    if (m_activeSessions.find(sessionId) == m_activeSessions.end()) {
        // 新建会话
        SessionInfo newSession;
        newSession.app_uid = app_uid;
        newSession.timestamp = format_timeval(ts);
        newSession.session_id = sessionId;
        newSession.protocol = protocol;
        newSession.src_ip = actualSrcIp;
        newSession.src_port = src_port;
        newSession.dst_ip = actualDesIp;
        newSession.dst_port = dst_port;
        newSession.size = 1;
        newSession.last_update_time = now;
        
        m_activeSessions[sessionId] = newSession;
        spdlog::info("New session created: {}", sessionId);
    } else {
        // 更新现有会话
        auto& session = m_activeSessions[sessionId];
        session.size++;
        session.last_update_time = now;
    }

    // 检查是否需要刷新
    if (m_activeSessions.size() >= MIN_SESSIONS_BEFORE_FLUSH || 
        now - m_lastFlushTime >= FLUSH_TIMEOUT_SECONDS) {
        shouldFlush = true;
    }

    sessionsLock.unlock(); // 释放锁以避免长时间持有
    
    // 需要刷新时，通知会话管理线程
    if (shouldFlush) {
        spdlog::info("Triggering flush: sessions={}, timeout={}s", 
                    m_activeSessions.size(), now - m_lastFlushTime);
        {
            std::lock_guard<std::mutex> pendingLock(m_pendingMutex);
            m_pendingCV.notify_one();
        }
    }
    
    return j;
       
       //spdlog::info("Parsed Session packet: session_id={},timestamp={}",
                    //sessionId, format_timeval(ts));
        // // 将会话信息添加到解析结果中
        // j["session_id"] = sessionId;
        // j["app_uid"] = app_uid;
        // j["protocol"] = protocol;
        // j["timestamp"] = format_timeval(ts);
        // j["src_ip"] = actualSrcIp;
        // j["des_ip"] = actualDesIp;
        // j["src_port"] = src_port;
        // j["des_port"] = dst_port;
        // j["session_packet_count"] = m_sessionCache[sessionId].size;
        
}


json PacketParser::parse_udp(const uint8_t* data, size_t len, const timeval& ts,
                              const std::string& src_ip, const std::string& des_ip,
                            const uint8_t* ip_header_ptr, size_t ip_header_len)
{
    if (len < sizeof(UDP_HEADER)) return {};
    const UDP_HEADER* udp = (const UDP_HEADER*)data;

    size_t udp_header_len = sizeof(UDP_HEADER);
    if (len < udp_header_len) return {};

    // 提取 UDP payload 数据
    const uint8_t* payload = data + udp_header_len;
    size_t payload_len = len - udp_header_len;

    json j;
    j["app_uid"] = app_uid;
    j["protocol"] = "UDP";

    j["timestamp"] = format_timeval(ts);
    j["src_ip"] = src_ip;
    j["des_ip"] = des_ip;
    j["src_port"] = ntohs(udp->src_port);
    j["des_port"] = ntohs(udp->des_port);
    j["packet_len"] = ntohs(udp->data_length);

        // 添加 header 字段：包括 IP 头 +UDP 头
    std::stringstream header_ss;
    for (size_t i = 0; i < ip_header_len + udp_header_len; ++i)
        header_ss << std::hex << std::setw(2) << std::setfill('0') << (int)(ip_header_ptr[i]);
    j["header"] = header_ss.str();

    // UDP 数据负载封装为十六进制字符串
    std::stringstream ss;
    for (size_t i = 0; i < payload_len; ++i) 
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)payload[i];
    }
    j["data"] = ss.str();

    // spdlog::info("Parsed UDP packet: timestamp={}, src_ip={}, des_ip={}, src_port={}, des_port={}, length={}",
    //             j["timestamp"].get<std::string>(),
    //             j["src_ip"].get<std::string>(), j["des_ip"].get<std::string>(),
    //             j["src_port"].get<int>(), j["des_port"].get<int>(),
    //             j["packet_len"].get<int>());
    return j;
}

json PacketParser::parse_dns(const uint8_t* data, size_t len, const timeval& ts,
                             const std::string& src_ip, const std::string& des_ip,
                             const uint8_t* ip_header_ptr, size_t ip_header_len) 
{
    if (len < 12) return {}; // DNS Header 至少 12 字节
    json j;
    j["app_uid"] = app_uid;
    j["protocol"] = "UDP";
    j["top_protocol"] = "DNS";
    j["timestamp"] = format_timeval(ts);

    if(des_ip=="192.168.31.172")
    {
        j["src_ip"] = src_ip;
        j["des_ip"] = m_src_ip;
    }
    else if(src_ip=="192.168.31.172")
    {
        
        j["src_ip"] = m_src_ip;
        j["des_ip"] = des_ip;
    }

    const UDP_HEADER* udp = reinterpret_cast<const UDP_HEADER*>(data - sizeof(UDP_HEADER));
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t des_port = ntohs(udp->des_port);
    j["src_port"] = src_port;
    j["des_port"] = des_port;

    uint16_t transaction_id = ntohs(*(uint16_t*)data);
    uint16_t flags = ntohs(*(uint16_t*)(data + 2));
    uint16_t qdcount = ntohs(*(uint16_t*)(data + 4));
    uint16_t ancount = ntohs(*(uint16_t*)(data + 6));
    // uint16_t nscount = ntohs(*(uint16_t*)(data + 8));
    // uint16_t arcount = ntohs(*(uint16_t*)(data + 10));

    j["transaction_id"] = transaction_id;
    j["qdcount"] = qdcount;
    j["ancount"] = ancount;

    size_t offset = 12;

    if (qdcount > 0 && offset < len) 
    {
        std::string qname;
        while (offset < len && data[offset] != 0) 
        {
            uint8_t label_len = data[offset++];
            if (label_len == 0 || offset + label_len > len) break;
            qname.append((const char*)(data + offset), label_len);
            qname.append(".");
            offset += label_len;
        }

        if (qname.empty()) {
            spdlog::warn("qname is empty, raw data may be malformed or truncated");
            return {};
        }

        ++offset; // skip null byte

        if (offset + 4 > len) return {};
        uint16_t qtype = ntohs(*(uint16_t*)(data + offset));
        uint16_t qclass = ntohs(*(uint16_t*)(data + offset + 2));
        offset += 4;

        std::stringstream ss;
        ss << qtype << " " << qclass << " " << qname;
        j["queries"] = ss.str();
    }
    // spdlog::info("Parsed DNS packet: timestamp={}, src_ip={}, des_ip={}, transaction_id={}",
    //             j["timestamp"].get<std::string>(),
    //             j["src_ip"].get<std::string>(), j["des_ip"].get<std::string>(),
    //             j["transaction_id"].get<int>());
    return j;
}

std::string PacketParser::format_timeval(const timeval& tv) 
{
    char buffer[64];
    std::time_t sec = tv.tv_sec;
    std::tm* t = std::localtime(&sec);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    std::ostringstream oss;
    oss << buffer << "." << std::setw(6) << std::setfill('0') << tv.tv_usec;
    return oss.str();
}

void PacketParser::start_storage() 
{
    m_storage_thread = std::thread([this]() {
        while(m_running) 
        {
            json packet;
            {
                std::unique_lock<std::mutex> lock(m_storage_mutex);
                m_storage_cv.wait(lock, [&]{ 
                    return !m_storage_queue.empty() || !m_running; });
                if (!m_running) break;
                
                packet = m_storage_queue.front();
                m_storage_queue.pop();
            }
            
            try {
                if (packet["protocol"] == "TCP") 
                {
                    // //spdlog::info("TCP Storage");
                    // if(m_mysql.store_tcp(packet)!=1)
                    // {
                    //     spdlog::error("TCP Storage failed");
                    // }
                }
                else if(packet["protocol"] == "UDP")
                {
                    //spdlog::info("DNS Storage");
                    if(m_mysql.store_dns(packet)!=1)
                    {
                        spdlog::error("UDP Storage failed");
                    }
                }
                
                // 其他协议处理...
            } catch (const std::exception& e) 
            {
                spdlog::error("Storage failed: {}", e.what());
            }
        }
    });
}

std::string PacketParser::parse_tcp_flags(uint8_t flags)
 {
    std::string s;
    if (flags & 0x01) s += "FIN ";
    if (flags & 0x02) s += "SYN ";
    if (flags & 0x04) s += "RST ";
    if (flags & 0x08) s += "PSH ";
    if (flags & 0x10) s += "ACK ";
    if (flags & 0x20) s += "URG ";
    return s;
}