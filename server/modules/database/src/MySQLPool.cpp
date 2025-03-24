#include "MySQLPool.h"
template class Singleton<MySQLPool>;

MySQLPool::MySQLPool()
    : m_host(nullptr), m_user(nullptr), m_passwd(nullptr), m_db(nullptr), m_port(0), m_maximumPoolSize(10), m_bStop(false)
{
}

MySQLPool::~MySQLPool()
{
    Close();
}

bool MySQLPool::Init(const std::string &host, const std::string &user, const std::string &passwd, const std::string &db, unsigned int port, unsigned int maximumPoolSize)
{
    try
    {
        m_host = host.c_str();
        m_user = user.c_str();
        m_passwd = passwd.c_str();
        m_db = db.c_str();
        m_port = port;
        m_maximumPoolSize = maximumPoolSize;

        for (int i = 0; i < 10; i++)
        {
            // 初始化mysql连接环境
            MYSQL *conn = mysql_init(nullptr);
            if (conn == nullptr)
            {
                spdlog::error("mysql_init error: {0}", mysql_error(conn));
                return false;
            }

            // 连接数据库服务器
            conn = mysql_real_connect(conn, m_host, m_user, m_passwd, m_db, m_port, NULL, 0);
            if (conn == nullptr)
            {
                spdlog::error("mysql_real_connect error: {0}", mysql_error(conn));
                return false;
            }

            m_connList.push(conn);
        }

        spdlog::info("MySQLPool init success, pool size: {0}", m_connList.size());

        return true;
    }
    catch (const std::exception &e)
    {
        spdlog::error("MySQLPool init error: {0}", e.what());
        return false;
    }
}

void MySQLPool::Close()
{
    m_bStop = true;
    m_cond.notify_all();

    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_connList.empty())
    {
        MYSQL *conn = m_connList.front();
        m_connList.pop();
        mysql_close(conn);
    }
}

MYSQL *MySQLPool::GetConnection()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]
                { 
                    if (m_bStop) 
                    {
                        return true;
                    }  
                    return !m_connList.empty(); });

    MYSQL *conn = m_connList.front();
    m_connList.pop();
    return conn;
}

void MySQLPool::ReleaseConn(MYSQL *conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connList.push(conn);
    m_cond.notify_one();
}