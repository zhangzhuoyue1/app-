
#include "ZmqSubscriber.h"
#include <gtest/gtest.h> // Include Google Test framework
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>

class ZmqSubscriberTest : public ::testing::Test {
protected:
    ZMQSubscriber subscriber{"tcp://localhost:5555"};
    
    void SetUp() override {
        subscriber.start();
    }
    
    void TearDown() override {
        subscriber.stop();
    }
};

TEST_F(ZmqSubscriberTest, ReceiveHttpMessage) {
    // 1. 先启动发送线程
    std::thread sender([]{
        zmq::context_t ctx;
        zmq::socket_t sock(ctx, ZMQ_PUB);
        sock.bind("tcp://*:5555");
        
        // 等待订阅者连接
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 发送测试消息
        sock.send(zmq::buffer(R"({"scheme":"http","content":"test_data"})"), 
                 zmq::send_flags::none);
    });

    // 2. 接收验证
    nlohmann::json packet;
    bool received = false;
    for (int i = 0; i < 10 && !received; ++i) { // 延长超时到10次尝试
        if (subscriber.get_http_packet(packet)) {
            received = true;
            
            //打印数据
            spdlog::info("Received HTTP packet: {}", packet.dump());

            spdlog::info("协议类型：{}", packet["scheme"].get<std::string>());
            spdlog::info("内容：{}", packet["content"].get<std::string>());

            EXPECT_EQ(packet["scheme"], "http");
            EXPECT_EQ(packet["content"], "test_data");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    EXPECT_TRUE(received) << "未能在5秒内收到测试消息";
    
    if (sender.joinable()) sender.join();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}