#include "../include/TrafficCapture.h"

TrafficCapture::TrafficCapture(const std::string strAppId, int port)
    : m_strAppId(strAppId), m_iPort(port), m_bIsCapture(false), m_cb(nullptr), m_pcapHandle(nullptr)
{
}

TrafficCapture::~TrafficCapture()
{
    StopCapture();
}

bool TrafficCapture::SetupIptablesRule()
{
    // 设置 iptables 规则
    const std::string cmd = "sudo iptables -t nat -A OUTPUT -p tcp -m owner --uid-owner " + m_strAppId + " -j REDIRECT --to-port " + std::to_string(m_iPort);
    if (system(cmd.c_str()) == -1)
    {
        spdlog::error("[TrafficCapture] setupIptablesRule failed");
        return false;
    }
    return true;
}

void TrafficCapture::ThreadCapture()
{
    spdlog::info("[TrafficCapture] ThreadCapture started");

    // 包装回调以访问成员变量
    auto wrapped_handler = [](u_char *user, const struct pcap_pkthdr *hdr, const u_char *data)
    {
        auto ctx = reinterpret_cast<TrafficCapture *>(user);
        if (ctx && ctx->m_cb && ctx->m_bIsCapture.load())
        {
            ctx->m_cb(user, hdr, data);
        }
    };

    // 开始抓包循环
    int rc = pcap_loop(m_pcapHandle, 0, wrapped_handler, reinterpret_cast<u_char *>(this));
    if (rc == -1)
    {
        spdlog::error("[TrafficCapture] pcap_loop failed: {}", pcap_geterr(m_pcapHandle));
    }

    spdlog::info("[TrafficCapture] pcap_loop started");
    // 自动清理状态
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bIsCapture.store(false);
}

bool TrafficCapture::StartCapture(HandlerCallBack cb)
{

    std::lock_guard<std::mutex> lock(m_mutex);
    // 启动捕获
    if (m_bIsCapture.load())
    {
        spdlog::warn("[TrafficCapture] StartCapture failed, already started");
        return false;
    }

    m_cb = cb;

    if (!SetupIptablesRule())
    {
        spdlog::error("[TrafficCapture] StartCapture failed, SetupIptablesRule failed");
        return false;
    }
    // 创建 pcap 句柄
    char errbuf[PCAP_ERRBUF_SIZE];
    m_pcapHandle = pcap_open_live("any", BUFSIZ, 1, 1000, errbuf);
    if (!m_pcapHandle)
    {
        spdlog::error("[TrafficCapture] StartCapture failed, pcap_open_live failed: {}", errbuf);
        return false;
    }

    // 编译过滤器
    std::string filter = "tcp port " + std::to_string(m_iPort);
    struct bpf_program fp;
    if (pcap_compile(m_pcapHandle, &fp, filter.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1)
    {
        spdlog::error("[TrafficCapture] StartCapture failed, pcap_compile failed: {}", pcap_geterr(m_pcapHandle));
        return false;
    }

    if (pcap_setfilter(m_pcapHandle, &fp) == -1)
    {
        spdlog::error("[TrafficCapture] StartCapture failed, pcap_setfilter failed: {}", pcap_geterr(m_pcapHandle));
        return false;
    }

    // 启动捕获线程
    m_bIsCapture.store(true);
    m_threadCapture = std::thread(&TrafficCapture::ThreadCapture, this);

    return true;
}

void TrafficCapture::StopCapture()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_bIsCapture.load())
        {
            return;
        }
        m_bIsCapture.store(false);
    }

    if (m_pcapHandle)
    {
        pcap_breakloop(m_pcapHandle); // 线程安全的中断方式
    }

    // 等待线程完成
    if (m_threadCapture.joinable())
    {
        m_threadCapture.join();
    }

    // 清理资源
    if (m_pcapHandle)
    {
        pcap_close(m_pcapHandle);
        m_pcapHandle = nullptr;
    }

    // 移除iptables规则
    std::string cmd = "iptables -D OUTPUT -m owner --cmd-owner " +
                      m_strAppId + " -j REDIRECT --to-port " +
                      std::to_string(m_iPort) + " 2>/dev/null";
    std::system(cmd.c_str());
}