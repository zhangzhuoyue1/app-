#include "TrafficCapture.h"

TrafficCapture::TrafficCapture(const std::string& app_id, const std::string& ip)
    : m_app_id(app_id)
    , m_target_ip(ip)
    ,m_running(false)
    , m_pcap_handle(nullptr)
    ,m_packet_parser(new PacketParser())
{
    
}

TrafficCapture::~TrafficCapture()
{
    stop_capture();
}

bool TrafficCapture::start_capture(int uid)
{
    if (m_running) return false;
    app_uid=uid;
    m_running = true;
    m_packet_parser->start(uid);
    m_capture_thread = std::thread(&TrafficCapture::thread_capture, this);

    // 启动解析器
    return true;
}

/*************  ✨ Windsurf Command ⭐  *************/
/// @brief 停止流量捕获
///
/// 该函数停止流量捕获过程，包括停止数据包解析器、
/// 中断 pcap 循环并关闭 pcap 句柄，同时确保捕获线程安全退出。

/*******  9e5dd3aa-e608-49d4-b30b-c30c48c892e3  *******/
void TrafficCapture::stop_capture()
{
    // 如果没有运行，则直接返回
    if (!m_running) return;

    m_running = false;

    m_packet_parser->stop(); // 停止解析器

    if (m_pcap_handle) 
    {
        pcap_breakloop(m_pcap_handle); // 使 pcap_loop 退出
    }

    if (m_capture_thread.joinable()) 
    {
        m_capture_thread.join();
    }

    if (m_pcap_handle) 
    {
        pcap_close(m_pcap_handle);
        m_pcap_handle = nullptr;
    }
}

void TrafficCapture::thread_capture()
{
    char errbuf[PCAP_ERRBUF_SIZE] = {0};

    // 打开 "any" 接口监听所有设备，可替换为具体接口如 "ens33"
    m_pcap_handle = pcap_open_live("ens33", 65536, 1, 100, errbuf);
    if (!m_pcap_handle) 
    {
        spdlog::error("pcap_open_live failed: {}", errbuf);
        return;
    }

    // 设置 BPF 过滤器
    if (!set_filter(m_target_ip)) 
    {
        spdlog::warn("未能正确设置 BPF 过滤器，所有流量将被捕获");
    }

    spdlog::info("开始捕获 IP [{}] 的数据包...", m_target_ip);

    // 阻塞进入捕获循环
    pcap_loop(m_pcap_handle, 0, pcap_callback, reinterpret_cast<u_char*>(this));
}

bool TrafficCapture::set_filter(const std::string& ip)
{
    // const std::string& ip="192.168.98.200";
    // 设置 BPF 过滤器
    std::string filter_expr = "host " + ip + 
                              " and not (host 192.168.98.200 or host 192.168.98.185)";
    struct bpf_program fp;

    if (pcap_compile(m_pcap_handle, &fp, filter_expr.c_str(), 1, PCAP_NETMASK_UNKNOWN) == -1) 
    {
        spdlog::error("pcap_compile failed: {}", pcap_geterr(m_pcap_handle));
        return false;
    }

    if (pcap_setfilter(m_pcap_handle, &fp) == -1) 
    {
        spdlog::error("pcap_setfilter failed: {}", pcap_geterr(m_pcap_handle));
        pcap_freecode(&fp);
        return false;
    }

    pcap_freecode(&fp);
    spdlog::info("成功设置 BPF 过滤器: {}", filter_expr);
    return true;
}

/// @brief  libpcap回调函数
/// @param user     // 用户数据指针
/// @param header   // 数据包头部    
//     struct pcap_pkthdr {
//     struct timeval ts;     // 时间戳：数据包被捕获的时间
//     bpf_u_int32 caplen;    // 实际捕获的长度（可能小于原始长度）
//     bpf_u_int32 len;       // 原始数据包的总长度
// };
/// @param packet   // 数据包数据
void TrafficCapture::pcap_callback(u_char* user, const struct pcap_pkthdr* header, const u_char* packet)
{
    auto* self = reinterpret_cast<TrafficCapture*>(user);
    
    // 时间戳：数据包被捕获的时间 
    auto now = header->ts; 
    if (self && self->m_running && header->caplen > 0) 
    {
        self->process_packet(now,packet, header->caplen);
    }
}

/// @brief  实际处理函数
/// @param data     // 数据包数据
/// @param length   // 数据包长度
void TrafficCapture::process_packet(const timeval& timestamp, const u_char* data, size_t length)
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    Packet packet;
    packet.timestamp = timestamp; // 设置时间戳
    packet.data.resize(length); // 设置数据包大小
    memcpy(packet.data.data(), data, length);
    m_packet_parser->push_raw_packet(packet); // 将数据包推入队列.emplace(data, data + length);
}


void TrafficCapture::get_net_devices()
{
    pcap_if_t* alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        spdlog::error("pcap_findalldevs failed: {}", errbuf);
        return;
    }

    for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
        spdlog::info("Device: {}", d->name);
        if (d->description) {
            spdlog::info("  Description: {}", d->description);
        }
    }

    pcap_freealldevs(alldevs);
}
