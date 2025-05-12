#pragma once
#include <cstdlib>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <pcap.h> //libpcap
#include <spdlog/spdlog.h>
#include <atomic>
#include <mutex>

// 功能 ： 捕获指定 app 的流量
//----------------------------------------------------------------
//  1. 通过配置为桥接模式捕获流量
//  2. 使用 libpcap 捕获端口流量
//----------------------------------------------------------------

/*
libpcap 回调函数
@param user 参数是 pcap_loop() 的第一个参数
@param const struct pcap_pkthdr* header 参数是一个指向捕获到的数据包的头部的指针
@param packet 参数是一个指向捕获到的数据包的内容的指针
*/
using HandlerCallBack = void (*)(u_char *, const struct pcap_pkthdr *, const u_char *);

class TrafficCapture
{
public:
    TrafficCapture(const std::string strAppId, const char *ip);
    ~TrafficCapture();

    bool StartCapture(HandlerCallBack cb); // 开始捕获
    void StopCapture();                    // 停止捕获
private:
    void ThreadCapture(); // 捕获线程
    bool SetupIptablesRule(); // 设置iptables规则
    void GetNetDevices(); // 获取网络设备

    const std::string m_app_id;      // app包名
    const char *m_ip;                // 模拟器ip
    std::atomic<bool> m_bis_capture; // 是否开始捕获
    HandlerCallBack m_cb;            // 回调函数
    pcap_t *m_pcap_handle;           // pcap句柄
    std::thread m_thread_capture;    // 捕获线程
    std::mutex m_mutex;              // 互斥锁
};