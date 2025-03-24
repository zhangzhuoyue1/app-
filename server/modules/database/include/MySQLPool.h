#pragma once

// Mysql连接池

#include <thread>
#include <mysql/mysql.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "Singleton.h"

class MySQLPool : public Singleton<MySQLPool>
{
    friend class Singleton<MySQLPool>;

public:
    MySQLPool();
    ~MySQLPool();
    /*
     * 初始化连接池
     * @param host 主机地址
     * @param user 用户名
     * @param passwd 密码
     * @param db 数据库名
     * @param port 端口，如果==0, 使用mysql的默认端口3306, !=0, 使用指定的这个端口
     * @param maximumPoolSize 连接池最大连接数
     * @return true: 初始化成功, false: 初始化失败
     */
    bool Init(const std::string &host, const std::string &user, const std::string &passwd, const std::string &db, unsigned int port, unsigned int maximumPoolSize);
    void Close();
    MYSQL *GetConnection();
    void ReleaseConn(MYSQL *conn);

private:
    const char *m_host;             // 主机地址
    const char *m_user;             // 用户名
    const char *m_passwd;           // 密码
    const char *m_db;               // 数据库名
    unsigned int m_port;            // 端口，如果==0, 使用mysql的默认端口3306, !=0, 使用指定的这个端口
    unsigned int m_maximumPoolSize; // 连接池最大连接数
    std::queue<MYSQL *> m_connList; // 连接队列
    std::mutex m_mutex;             // 互斥锁
    std::condition_variable m_cond; // 条件变量
    std::atomic<bool> m_bStop;      // 停止标志
};