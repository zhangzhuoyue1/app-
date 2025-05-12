#pragma once

// 包含 MySQL Connector/C++ 所需头文件
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

#include <memory>               // std::unique_ptr
#include <queue>                // std::queue
#include <mutex>                // std::mutex, std::unique_lock
#include <condition_variable>   // std::condition_variable
#include <atomic>               // std::atomic
#include <iostream>             // std::cerr

class MySqlPool:public std::enable_shared_from_this<MySqlPool> {
    // 友元类，允许 MySQLDAO 访问私有成员
    friend class MySQLDAO;

public:
    /**
     * 构造函数：初始化连接池
     * @param url      数据库地址，如 "tcp://127.0.0.1:3306"
     * @param user     数据库用户名
     * @param pass     数据库密码
     * @param schema   默认使用的数据库名
     * @param poolSize 连接池中连接的数量
     */
    MySqlPool(const std::string& url, const std::string& user,
              const std::string& pass, const std::string& schema,
              int poolSize)
        : m_url(url)
        , m_user(user)
        , m_pass(pass)
        , m_schema(schema)
        , m_poolSize(poolSize)
        , b_stop_(false) 
    {
        try
         {
            // 初始化连接池中的连接
            for (int i = 0; i < m_poolSize; ++i) 
            {
                // 获取 MySQL 驱动实例
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();

                // 建立连接
                std::unique_ptr<sql::Connection> con(driver->connect(m_url, m_user, m_pass));
                
                // 设置默认数据库
                con->setSchema(m_schema);

                // 设置字符集（避免乱码）
                con->setClientOption("CHARSET", "utf8mb4");

                // 将连接放入连接池队列中
                m_pool.push(std::move(con));
            }
        } 
        catch (sql::SQLException& e) 
        {
            std::cerr << "[MySqlPool] 初始化失败: " << e.what() << std::endl;
        }
    }

    /**
     * 获取连接：线程安全，若连接池为空则等待
     * @return 数据库连接指针（unique_ptr），调用者需负责归还
     */
    std::unique_ptr<sql::Connection> get_connection() 
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // 等待直到有连接可用或停止标志触发
        m_cond.wait(lock, [this] {
            return b_stop_ || !m_pool.empty();
        });

        // 若池已关闭或空，返回空指针
        if (b_stop_ || m_pool.empty()) 
        {
            return nullptr;
        }

        // 获取连接
        std::unique_ptr<sql::Connection> con = std::move(m_pool.front());
        m_pool.pop();
        return con;
    }

    /**
     * 归还连接：将连接放回池中
     * @param con 数据库连接指针（unique_ptr）
     */
    void return_connection(std::unique_ptr<sql::Connection> con) 
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!b_stop_) {
            m_pool.push(std::move(con));
            m_cond.notify_one();  // 通知等待线程
        }
    }

    /**
     * 主动关闭连接池，唤醒所有等待线程
     */
    void Close() 
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            b_stop_ = true;  // 设置关闭标志
        }
        m_cond.notify_all();  // 唤醒所有可能在等待连接的线程
    }

    /**
     * 析构函数：清理资源
     * 自动清空连接池（连接由 unique_ptr 管理，自动关闭）
     */
    ~MySqlPool() 
    {
        Close();  // 确保池关闭
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_pool.empty()) {
            m_pool.pop();  // 连接会自动释放
        }
    }

private:
    // 数据库连接信息
    std::string m_url;      // 数据库地址
    std::string m_user;     // 用户名
    std::string m_pass;     // 密码
    std::string m_schema;   // 默认数据库
    int m_poolSize;         // 连接池大小

    // 连接池核心结构
    std::queue<std::unique_ptr<sql::Connection>> m_pool; // 连接队列

    // 多线程同步机制
    std::mutex m_mutex;                 // 用于保护队列访问的互斥锁
    std::condition_variable m_cond;     // 用于线程等待和通知
    std::atomic<bool> b_stop_;         // 标记连接池是否关闭
};
