#pragma once
#include <string>
#include <thread>
#include <pcap.h>
#include <spdlog/spdlog.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>
#include "PacketParser.h"

/**
 * @brief 使用 libpcap 实现指定 IP 流量捕获与队列缓存，供解析模块调用
 */
class TrafficCapture
{
public:
    /**
     * @param app_id 应用标识，仅供记录使用
     * @param ip 要监听的目标 IP（可为 Android 模拟器 IP）
     */
    TrafficCapture(const std::string& app_id, const std::string& ip);
    ~TrafficCapture();

    bool                start_capture(int uid);                     //开始抓包
    void                stop_capture();                      //停止抓包
    void                process_packet(const timeval&, const u_char*, size_t); //实际处理函数
    bool                set_filter(const std::string& ip);   //设置 BPF 过滤器

private:
    void                thread_capture();                    // 工作线程入口
    static void         pcap_callback(u_char*, const struct pcap_pkthdr*, const u_char*); // libpcap回调
    void                get_net_devices();                    // 打印设备列表（调试用）

    const std::string               m_app_id;           // 应用id
    const std::string               m_target_ip;        // 目标ip

    std::atomic<bool>               m_running;          // 运行标志
    pcap_t*                         m_pcap_handle;      // libpcap 句柄
    std::thread                     m_capture_thread;   // 工作线程

    std::mutex                      m_queue_mutex;      // 互斥锁
    PacketParser*                   m_packet_parser;     // 数据包解析器
    int                             app_uid=10001;
};
