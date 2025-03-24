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
//  1. 通过 iptables 设置过滤规则，并将流量重定向到本地端口
//  2. 使用 libpcap 捕获端口流量
//----------------------------------------------------------------

// libpcap 回调函数
// u_char* user 参数是 pcap_loop() 的第一个参数
// const struct pcap_pkthdr* header 参数是一个指向捕获到的数据包的头部的指针
// const u_char* packet 参数是一个指向捕获到的数据包的内容的指针
using HandlerCallBack = void (*)(u_char *, const struct pcap_pkthdr *, const u_char *);

class TrafficCapture
{
public:
    TrafficCapture(const std::string strAppId, int port = 4999);
    ~TrafficCapture();

    bool StartCapture(HandlerCallBack cb); // 开始捕获
    void StopCapture();                    // 停止捕获
private:
    void ThreadCapture();     // 捕获线程
    bool SetupIptablesRule(); // 设置iptables规则

    const std::string m_strAppId;   // app包名
    int m_iPort;                    // 转发端口号
    std::atomic<bool> m_bIsCapture; // 是否开始捕获
    HandlerCallBack m_cb;           // 回调函数
    pcap_t *m_pcapHandle;           // pcap句柄
    std::thread m_threadCapture;    // 捕获线程
    std::mutex m_mutex;             // 互斥锁
};