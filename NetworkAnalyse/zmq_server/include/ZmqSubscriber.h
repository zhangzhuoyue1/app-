#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include <queue>
#include <mutex>


/// @brief ZMQSubscriber类用于订阅ZMQ消息并将其存储在线程安全的队列中
/// @details
///使用ZMQ订阅消息  
class ZMQSubscriber {
public:
    ZMQSubscriber(const std::string& address = "tcp://localhost:5555");
    ~ZMQSubscriber();

    // 启停控制接口
    void                start();
    void                stop();
    bool                get_http_packet(nlohmann::json& packet);
    bool                get_https_packet(nlohmann::json& packet);

private:
    void                worker_thread();                               // 工作线程
    void                process_message(const nlohmann::json& j);      // 处理消息

    zmq::context_t      m_m_ctx;    // zmq上下文
    zmq::socket_t       m_socket;   // zmq套接字
    std::thread         m_worker;    // 工作线程
    std::atomic<bool>   m_running;    // 运行标志
    
    // 双队列存储
    std::queue<nlohmann::json> m_http_queue;  // http队列
    std::queue<nlohmann::json> m_https_queue; // https队列
    std::mutex          m_queue_mutex;  //队列互斥锁
};