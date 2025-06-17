#pragma once

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

class JsonSender {
public:
    JsonSender(const std::string& address = "tcp://0.0.0.0:5556");
    ~JsonSender();

    void start();
    void stop();

    // 提供接口用于向队列中存储 JSON 数据
    void enqueue_json(const nlohmann::json& json_data);

private:
    void worker_thread();

    zmq::context_t m_ctx;
    zmq::socket_t m_socket;
    std::thread m_worker;
    std::atomic<bool> m_running{false};

    std::queue<nlohmann::json> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};