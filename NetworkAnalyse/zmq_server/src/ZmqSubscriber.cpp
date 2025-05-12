#include <ZmqSubscriber.h>
#include <spdlog/spdlog.h>

ZMQSubscriber::ZMQSubscriber(const std::string& address)
    : m_m_ctx(1)
    , m_socket(m_m_ctx, zmq::socket_type::sub)
    , m_running(false) 
{
    m_socket.connect(address);
    m_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0); // 订阅所有消息
}

ZMQSubscriber::~ZMQSubscriber() 
{
    stop();
}

// 开始订阅
void ZMQSubscriber::start() 
{   
    // 如果已经在运行，则不再启动新的线程
    if (!m_running.exchange(true)) 
    {
        m_running = true;
        m_worker = std::thread(&ZMQSubscriber::worker_thread, this);
    }
}

//停止订阅
void ZMQSubscriber::stop() 
{
    // 如果正在运行，则停止线程
    if (m_running.exchange(false)) 
    {
        if (m_worker.joinable()) 
        {
            m_worker.join();
        }
        m_socket.close();
        m_running = false;
    }
}

void ZMQSubscriber::worker_thread() {
    while(m_running)
     {
        // 接收消息
        zmq::message_t msg;
        if(m_socket.recv(msg, zmq::recv_flags::dontwait)) 
        {
            try 
            {
                auto j = nlohmann::json::parse(msg.to_string());
                process_message(j);
            } catch (const std::exception& e) 
            {
                // 错误处理逻辑
                spdlog::error("Failed to parse message: {}", e.what());
            }
        } else 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void ZMQSubscriber::process_message(const nlohmann::json& j) 
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    // 处理消息并将其存储到相应的队列中
    if(j["scheme"] == "http") 
    {
        m_http_queue.push(j);
    } 
    else if(j["scheme"] == "https") 
    {
        m_https_queue.push(j);
    }
}

// 获取http队列数据
bool ZMQSubscriber::get_http_packet(nlohmann::json& packet) 
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if(m_http_queue.empty()) 
        return false;
    
    packet = m_http_queue.front();
    m_http_queue.pop();
    return true;
}

// 获取https队列数据
bool ZMQSubscriber::get_https_packet(nlohmann::json& packet) 
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if(m_https_queue.empty()) 
        return false;
    
    packet = m_https_queue.front();
    m_https_queue.pop();
    return true;
}