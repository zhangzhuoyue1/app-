#pragma once

// Redis 连接池

#include <thread>
#include <hiredis/hiredis.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "Singleton.h"

class RedisPool : public Singleton<RedisPool>
{
    friend class Singleton<RedisPool>;

public:
    RedisPool();
    ~RedisPool();

    /*
    初始化连接池
    @param host 主机地址
    @param port 端口
    @param maximumPoolSize 连接池最大连接数
    @return true: 初始化成功, false: 初始化失败
    */
    bool Init(const std::string ip, const std::string port, const int poolSize);
    void Close();

    redisContext *GetConnection();
    void ReleaseConn(redisContext *conn);

private:
    const char *m_host;                    // 主机地址
    unsigned int m_port;                   // 端口
    unsigned int m_maximumPoolSize;        // 连接池最大连接数
    std::queue<redisContext *> m_connList; // 连接队列
    std::mutex m_mutex;                    // 互斥锁
};