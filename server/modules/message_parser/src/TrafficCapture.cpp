#include "../include/TrafficCapture.h"

TrafficCapture::TrafficCapture(const std::string strAppId, const char *ip)
    : m_app_id(strAppId), m_ip(ip),
      m_bis_capture(false),
      m_cb(nullptr),
      m_pcap_handle(nullptr)
{
}

TrafficCapture::~TrafficCapture()
{
    StopCapture();
}

bool TrafficCapture::StartCapture(HandlerCallBack cb)
{
    if (m_bis_capture)
    {
        return false;
    }

    m_cb = cb;
    m_bis_capture = true;
    m_thread_capture = std::thread(&TrafficCapture::ThreadCapture, this);

    return true;
}

void TrafficCapture::StopCapture()
{
    if (!m_bis_capture)
    {
        return;
    }

    m_bis_capture = false;
    if (m_thread_capture.joinable())
    {
        m_thread_capture.join();
    }
}

void TrafficCapture::ThreadCapture()
{
    // SetupIptablesRule();

    char errbuf[PCAP_ERRBUF_SIZE];
    // 打开一个用于捕获数据的网络接口，并且返回用于捕获网络数据包的数据包捕获描述字
    m_pcap_handle = pcap_open_live("any", 65535, 1, 1000, errbuf);
    if (m_pcap_handle == nullptr)
    {
        spdlog::error("pcap_open_live failed: {}", errbuf);
        return;
    }

    //设置过滤规则
}

void TrafficCapture::GetNetDevices()
{
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        spdlog::error("pcap_findalldevs failed: {}", errbuf);
        return;
    }

    for (pcap_if_t *d = alldevs; d != nullptr; d = d->next)
    {
        spdlog::info("Device: {}", d->name);
        if (d->description)
        {
            spdlog::info("Description: {}", d->description);
        }
        else
        {
            spdlog::info("No description available");
        }
    }

    pcap_freealldevs(alldevs);
}

bool TrafficCapture::SetupIptablesRule()
{
    std::string cmd = "iptables -t nat -A PREROUTING -p tcp -s " + std::string(m_ip) + " -j REDIRECT --to-port 12345";
    int ret = system(cmd.c_str());
    if (ret != 0)
    {
        spdlog::error("SetupIptablesRule failed: {}", ret);
        return false;
    }

    return true;
}