#include "JsonSender.h"
#include <iostream>

JsonSender::JsonSender(const std::string& address)
    : m_ctx(1), m_socket(m_ctx, zmq::socket_type::pub) {
    m_socket.bind(address);
}

JsonSender::~JsonSender() {
    stop();
}

void JsonSender::start() {
    m_running = true;
    m_worker = std::thread(&JsonSender::worker_thread, this);
}

void JsonSender::stop() {
    m_running = false;
    // 唤醒工作线程，使其能够检测到停止信号
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cv.notify_all();
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
    m_socket.close();
}

// 向队列中添加 JSON 数据
void JsonSender::enqueue_json(const nlohmann::json& json_data) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(json_data);
    }
    m_cv.notify_one(); // 通知工作线程队列中有数据
}

void JsonSender::worker_thread()    {
 while (m_running) {
        nlohmann::json json_data;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // 等待队列中有数据或者停止信号被触发
            m_cv.wait(lock, [this] {
                return !m_running || !m_queue.empty();
            });
            if (!m_running && m_queue.empty()) {
                break; // 如果停止信号被触发且队列为空，退出循环
            }
            json_data = std::move(m_queue.front());
            m_queue.pop();
        }
        try {
            m_socket.send(zmq::buffer(json_data.dump()), zmq::send_flags::none);
        } catch (const std::exception& e) {
            std::cerr << "Error sending JSON data: " << e.what() << std::endl;
        }
    }
}