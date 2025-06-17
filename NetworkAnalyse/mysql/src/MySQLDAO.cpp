#include "MySQLDAO.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <spdlog/spdlog.h>

std::string format_mysql_datetime(const std::string& iso_time) {
    if (iso_time.empty()) return "";

    // 示例："2025-05-25T07:50:41.520134Z"
    std::string formatted = iso_time;
    size_t t_pos = formatted.find('T');
    if (t_pos != std::string::npos) {
        formatted[t_pos] = ' ';
    }
    size_t dot_pos = formatted.find('.');
    if (dot_pos != std::string::npos) {
        formatted = formatted.substr(0, dot_pos); // 去掉毫秒
    }
    size_t z_pos = formatted.find('Z');
    if (z_pos != std::string::npos) {
        formatted = formatted.substr(0, z_pos);
    }
    return formatted;
}


MySQLDAO::MySQLDAO(const std::string& url="192.168.98.185:3308", const std::string& user="root",
                   const std::string& pass="123456", const std::string& schema="appnetworkanalyse", int poolSize=10)
{
    m_pool = std::make_unique<MySqlPool>(url, user, pass, schema, poolSize);
}

MySQLDAO::MySQLDAO()
{
     m_pool = std::make_unique<MySqlPool>("192.168.98.185:3308", "root", "123456", "appnetworkanalyse", 10);
}
MySQLDAO::~MySQLDAO() {
    m_pool->Close();
}

// 注册用户,返回 1 成功，0 重复用户名/邮箱，-1 出错
int MySQLDAO::reg_user(const std::string& name, const std::string& email, const std::string& pwd) 
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try 
    {
        // 检查用户名或邮箱是否已存在
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT COUNT(*) FROM users WHERE username=? OR email=?"));
        checkStmt->setString(1, name);
        checkStmt->setString(2, email);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        res->next();
        if (res->getInt(1) > 0)
        {
            return 0;  // 用户已存在
        }

        // 插入用户
        std::unique_ptr<sql::PreparedStatement> insertStmt(
            conn->prepareStatement("INSERT INTO users (username, password, email, user_type) VALUES (?, ?, ?, 'user')"));
        insertStmt->setString(1, name);
        insertStmt->setString(2, pwd);  // 实际使用中应为哈希
        insertStmt->setString(3, email);
        insertStmt->execute();
        return 1;
    }
     catch (...) 
    {
        return -1;
    }
    m_pool->return_connection(std::move(conn));
}

// 删除用户,返回 1 成功，0 用户不存在，-1 出错
int MySQLDAO::del_user_by_name(const std::string& name) 
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("DELETE FROM users WHERE username=?"));
        stmt->setString(1, name);
        int rows = stmt->executeUpdate();
        return rows > 0 ? 1 : 0;
    } 
    catch (...) 
    {
        return -1;
    }
    m_pool->return_connection(std::move(conn));
}

int MySQLDAO::update_user(const UserInfo& user)
{
    try {
        auto conn = m_pool->get_connection();
        std::string sql = "UPDATE users SET email=?, password=?, user_type=? WHERE username=?";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));

        stmt->setString(1, user.email);
        stmt->setString(2, user.pwd);
        stmt->setString(3, user.type);
        stmt->setString(4, user.name);

        int affected_rows = stmt->executeUpdate();
        return affected_rows > 0 ? 1 : 0;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "MySQLDAO::update_user error: " << e.what() << std::endl;
        return -1;
    }
}


// 查询用户信息
UserInfo MySQLDAO::get_user_by_name(const std::string& name) 
{
    auto conn = m_pool->get_connection();
    if (!conn)  return UserInfo();

    try 
    {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("SELECT username, email, user_type FROM users WHERE username=?"));
        stmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        if (res->next()) 
        {
            UserInfo user;
            user.name = res->getString("username");
            user.email = res->getString("email");
            user.type = res->getString("user_type");
            return user;
        } 
        else 
        {
            return UserInfo();  // 用户不存在
        }
        // 归还连接
        m_pool->return_connection(std::move(conn));
    } 
    catch (...) 
    {
        return UserInfo(); // 出错或异常
    }
}

// 添加用户（支持自定义角色），返回 1 成功，0 重复用户名/邮箱，-1 出错
int MySQLDAO::add_user_with_role(const std::string& name, const std::string& email, const std::string& pwd, const std::string& role)
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try 
    {
        // 检查用户名或邮箱是否已存在
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT COUNT(*) FROM users WHERE username=? OR email=?"));
        checkStmt->setString(1, name);
        checkStmt->setString(2, email);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        res->next();
        if (res->getInt(1) > 0)
        {
            m_pool->return_connection(std::move(conn));
            return 0;  // 用户已存在
        }

        // 插入用户（支持传入角色）
        std::unique_ptr<sql::PreparedStatement> insertStmt(
            conn->prepareStatement("INSERT INTO users (username, password, email, user_type) VALUES (?, ?, ?, ?)"));
        insertStmt->setString(1, name);
        insertStmt->setString(2, pwd);  // 实际中应加密哈希
        insertStmt->setString(3, email);
        insertStmt->setString(4, role);
        insertStmt->execute();

        m_pool->return_connection(std::move(conn));
        return 1;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Error in add_user_with_role: {}", e.what());
        m_pool->return_connection(std::move(conn));
        return -1;
    }
}




int MySQLDAO::store_tcp(const json& j)
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try {
        std::string sql = R"(
            INSERT INTO tcp_packets (
                app_uid, timestamp, src_ip, des_ip,
                src_port, des_port, sequence, ack, flags, packet_len,
                data, header
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));

        // 必填字段
        if (j.contains("app_uid")) 
        {
            if (j["app_uid"].is_number_integer()) 
            {
            stmt->setInt(1, j["app_uid"].get<int>());
            } 
            else if (j["app_uid"].is_string()) 
            {
            stmt->setInt(1, std::stoi(j["app_uid"].get<std::string>()));
            } 
        }
        stmt->setString(2, j.value("timestamp", ""));
        stmt->setString(3, j.value("src_ip", ""));
        stmt->setString(4, j.value("des_ip", ""));
        stmt->setInt(5, j.value("src_port", 0));
        stmt->setInt(6, j.value("des_port", 0));

        // 可选字段（null处理）
        if (j.contains("sequence"))
            stmt->setUInt64(7, j.at("sequence").get<uint64_t>());
        else
            stmt->setNull(7, sql::DataType::BIGINT);

        if (j.contains("ack"))
            stmt->setUInt64(8, j.at("ack").get<uint64_t>());
        else
            stmt->setNull(8, sql::DataType::BIGINT);

        if (j.contains("flags"))
            stmt->setString(9, j.at("flags").get<std::string>()); // 匹配 
        else
            stmt->setNull(9, sql::DataType::VARCHAR);

        stmt->setInt(10, j.value("packet_len", 0));
        stmt->setString(11, j.value("data", ""));
        stmt->setString(12, j.value("header", ""));

        stmt->execute();

        // 如果插入成功，返回 1，失败返回 -1
        m_pool->return_connection(std::move(conn));
        return 1;

    } catch (const std::exception& e) {
        m_pool->return_connection(std::move(conn));
        spdlog::error("Error storing TCP packet: {}", e.what());
        return -1;
    }
}

int MySQLDAO::store_udp(const json& j) 
{
    //spdlog::info("store_udp");

    // 字段检查
    std::vector<std::string> required_fields = {
        "app_uid", "timestamp", "src_ip", "des_ip",
        "src_port", "des_port", "packet_len", "data", "header"
    };

    for (const auto& field : required_fields) {
        if (!j.contains(field) || j[field].is_null()) {
            spdlog::error("JSON missing or null field: {}", field);
            spdlog::error("Received JSON: {}", j.dump());
            return -1;
        }
    }

    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try 
    {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(
                "INSERT INTO udp_packets "
                "(app_uid, timestamp, src_ip, des_ip, src_port, des_port, packet_len, data, header) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
            )
        );

        stmt->setInt(1, j.value("app_uid", -1));
        stmt->setString(2, j.value("timestamp", ""));
        stmt->setString(3, j.value("src_ip", ""));
        stmt->setString(4, j.value("des_ip", ""));
        stmt->setInt(5, j.value("src_port", 0));
        stmt->setInt(6, j.value("des_port", 0));
        stmt->setInt(7, j.value("packet_len", 0));
        stmt->setString(8, j.value("data", ""));
        stmt->setString(9, j.value("header", ""));

        stmt->execute();

        m_pool->return_connection(std::move(conn));
        return 1;
    } 
    catch (const std::exception& e) 
    {
        spdlog::error("MySQL insert error: {}", e.what());
        m_pool->return_connection(std::move(conn));
        return -1;
    }
}

int MySQLDAO::store_dns(const json& j)
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try {
        std::string sql = R"(
            INSERT INTO dns_packets (
                app_uid, timestamp, src_ip, src_port,des_ip,des_port,
                transaction_id, qdcount, ancount, queries
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));

        // app_uid 支持整型和字符串
        stmt->setInt(1, j.value("app_uid", 0));
        stmt->setString(2, j.value("timestamp", ""));
        stmt->setString(3, j.value("src_ip", ""));
        stmt->setInt(4, j.value("src_port", 0));
        stmt->setString(5, j.value("des_ip", ""));
        stmt->setInt(6, j.value("des_port", 0));

        stmt->setInt(7, j.value("transaction_id", 0));
        stmt->setInt(8, j.value("qdcount", 0));
        stmt->setInt(9, j.value("ancount", 0));

    
        stmt->setString(10, j.value("queries", ""));

        stmt->execute();

        m_pool->return_connection(std::move(conn));
        return 1;
    } catch (const std::exception& e) {
        m_pool->return_connection(std::move(conn));
        spdlog::error("Error storing DNS packet: {}", e.what());
        return -1;
    }
}

int MySQLDAO::store_icmp(const json& j) 
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try 
    {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("INSERT INTO icmp_packets (app_uid,timestamp, src_ip, des_ip,type, code, checksum, data,header) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        stmt->setInt(1, j["app_uid"].get<int>());
        stmt->setString(2, j["timestamp"].get<std::string>());
        stmt->setString(3, j["src_ip"].get<std::string>());
        stmt->setString(4, j["des_ip"].get<std::string>());
        stmt->setInt(5, j["type"].get<int>());
        stmt->setInt(6, j["code"].get<int>());
        stmt->setInt(7, j["checksum"].get<int>());
        stmt->setString(8, j["data"].get<std::string>());
        stmt->setString(9, j["header"].get<std::string>());
        stmt->execute();
        m_pool->return_connection(std::move(conn));
        return 1;

    } 
    catch (...) 
    {
        m_pool->return_connection(std::move(conn));
        return -1;
    }
}


int MySQLDAO::store_http(const json& j) 
{
    spdlog::info("store_http");
    auto conn = m_pool->get_connection();
    if (!conn) return -1;
    try {
        static const std::string sql = R"(
            INSERT INTO https_packets (
                app_uid, type, timestamp, src_ip, src_port,
                dst_ip, dst_port, http_version,
                method, path, length, content
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        auto stmt = std::unique_ptr<sql::PreparedStatement>(
            conn->prepareStatement(sql)
        );
        int idx=1;
        stmt->setInt   (idx++, j.value("app_uid", 0));
        stmt->setString(idx++, j.value("type", ""));
        stmt->setString(idx++, j.value("timestamp", ""));
        stmt->setString(idx++, j.value("src_ip", ""));
        stmt->setInt   (idx++, j.value("src_port", 0));
        stmt->setString(idx++, j.value("dst_ip", ""));
        stmt->setInt   (idx++, j.value("dst_port", 0));
        stmt->setString(idx++, j.value("http_version", ""));
        stmt->setString(idx++, j.value("method", ""));
        stmt->setString(idx++, j.value("path", ""));
        stmt->setInt   (idx++, j.value("length", 0));
        stmt->setString(idx++, j.value("content", ""));
        stmt->execute();
        m_pool->return_connection(std::move(conn));
        return 1;
    } catch (const std::exception& e) 
    {
        spdlog::error("Error storing HTTP packet: {}", e.what());
        m_pool->return_connection(std::move(conn));
        return -1;
    }
}

int MySQLDAO::store_app_info_if_not_exists(int app_uid, const std::string& package_name)
{
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try {
        // 1. 检查是否存在对应 UID 或包名的记录
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT COUNT(*) FROM applications WHERE app_uid = ? OR package_name = ?"));
        checkStmt->setInt(1, app_uid);
        checkStmt->setString(2, package_name);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        res->next();

        if (res->getInt(1) > 0) {
            m_pool->return_connection(std::move(conn));
            return 0;  // 已存在，不插入
        }

        // 2. 插入新记录
        std::unique_ptr<sql::PreparedStatement> insertStmt(
            conn->prepareStatement("INSERT INTO applications (app_uid, package_name) VALUES (?, ?)"));
        insertStmt->setInt(1, app_uid);
        insertStmt->setString(2, package_name);
        insertStmt->execute();

        m_pool->return_connection(std::move(conn));
        return 1;  // 插入成功

    } catch (const std::exception& e) {
        spdlog::error("store_app_info_if_not_exists error: {}", e.what());
        m_pool->return_connection(std::move(conn));
        return -1;  // 异常
    }
}


bool MySQLDAO::insert_http_flow_info(const HttpFlowInfo& info) {
    auto conn = m_pool->get_connection();
    if (!conn) return false;
    try {


        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(R"(
                INSERT INTO http_flow_info (
                    app_uid,flow_id, timestamp, src_ip, src_port, dst_ip, dst_port, protocol,
                    top_protocol, http_version, method, host, url, status, content_type
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )")
        );

        stmt->setInt(1, info.app_uid);                  // 对应app_uid
        stmt->setString(2, info.flow_id);               // 对应flow_id
        stmt->setString(3, format_mysql_datetime(info.start_time)); // 对应timestamp
        stmt->setString(4, info.src_ip);                // 对应src_ip
        stmt->setInt(5, info.src_port);                 // 对应src_port
        stmt->setString(6, info.dst_ip);                // 对应dst_ip
        stmt->setInt(7, info.dst_port);                 // 对应dst_port
        stmt->setString(8, info.protocol);              // 对应protocol
        stmt->setString(9, info.top_protocol);          // 对应top_protocol
        stmt->setString(10, info.http_version);         // 对应http_version
        stmt->setString(11, info.method);               // 对应method
        stmt->setString(12, info.host);                 // 对应host (之前跳过了)
        stmt->setString(13, info.url);                  // 对应url
        stmt->setInt(14, info.status_code);             // 对应status
        stmt->setString(15, info.content_type);         // 对应content_type

        stmt->executeUpdate();
        m_pool->return_connection(std::move(conn));
        return true;

    } catch (const sql::SQLException& e) {
        m_pool->return_connection(std::move(conn));
        spdlog::error("insert_http_flow_info error: {}", e.what());
        return false;
    }
}

bool MySQLDAO::insert_http_packet(const HttpPacket& packet) {
    auto conn = m_pool->get_connection();
    if (!conn) return false;
    try 
    {

        // 验证并转换direction值为ENUM允许的值
        std::string direction;
        if (packet.type == "request") {
            direction = "request";
        } else if (packet.type == "response") {
            direction = "response";
        } else {
            // 未知类型，默认设为request或记录错误
            direction = "request";
            spdlog::error("Invalid direction value: '{}', using default 'request'", packet.type);
        }

        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(R"(
                INSERT INTO http_packets (
                    flow_id, direction, protocol, timestamp, headers, body, content_type, length
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            )")
        );

        stmt->setString(1, packet.flow_id);
        stmt->setString(2, direction); // 使用验证后的direction值
        stmt->setString(3, packet.top_protocol);
        stmt->setString(4, format_mysql_datetime(packet.timestamp));
        stmt->setString(5, packet.headers.dump());
        stmt->setString(6, packet.body);
        stmt->setString(7, packet.content_type);
        stmt->setInt(8, packet.length);

        stmt->executeUpdate();

        m_pool->return_connection(std::move(conn));
        return true;

    } catch (const sql::SQLException& e) {
        m_pool->return_connection(std::move(conn));
        spdlog::error("insert_http_packet error: {}", e.what());
        return false;
    }
}


int MySQLDAO::insert_or_update_session_info(const SessionInfo& session) {
    auto conn = m_pool->get_connection();
    if (!conn) return -1;

    try 
    {
        std::string check_sql = "SELECT COUNT(*) FROM session_info WHERE session_id = ?";
        std::unique_ptr<sql::PreparedStatement> check_stmt(conn->prepareStatement(check_sql));
        check_stmt->setString(1, session.session_id);
        std::unique_ptr<sql::ResultSet> res(check_stmt->executeQuery());
        res->next();
        bool exists = res->getInt(1) > 0;

        if (!exists) {
            std::string insert_sql = R"(
                INSERT INTO session_info (
                    app_uid, timestamp, session_id, protocol,
                    src_ip, src_port, dst_ip, dst_port, packet_count
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            )";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(insert_sql));
            stmt->setInt(1, session.app_uid);
            stmt->setString(2, format_mysql_datetime(session.timestamp));
            stmt->setString(3, session.session_id);
            stmt->setString(4, session.protocol);
            stmt->setString(5, session.src_ip);
            stmt->setInt(6, session.src_port);
            stmt->setString(7, session.dst_ip);
            stmt->setInt(8, session.dst_port);
            stmt->setInt(9,session.size);
            stmt->execute();
        } else {
            std::string update_sql = "UPDATE session_info SET packet_count = packet_count + ? WHERE session_id = ?";
            std::unique_ptr<sql::PreparedStatement> update_stmt(conn->prepareStatement(update_sql));
            update_stmt->setInt(1, static_cast<int>(session.size)); // 增量更新
            update_stmt->setString(2, session.session_id);
            update_stmt->execute();
        }

        m_pool->return_connection(std::move(conn));
        return 1;
    } 
    catch (const std::exception& e) 
    {
        m_pool->return_connection(std::move(conn));
        spdlog::error("Error inserting/updating session_info: {}", e.what());
        return -1;
    }
}

// // 插入 session_resources 表
// int MySQLDAO::insert_session_resource(const std::string& session_id, const std::string& resource) {
//     auto conn = m_pool->get_connection();
//     if (!conn) return -1;

//     try {
//         std::string sql = R"(
//             INSERT INTO session_resources (session_id, resource) VALUES (?, ?)
//         )";
//         std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
//         stmt->setString(1, session_id);
//         stmt->setString(2, resource);
//         stmt->execute();

//         m_pool->return_connection(std::move(conn));
//         return 1;
//     } catch (const std::exception& e) {
//         m_pool->return_connection(std::move(conn));
//         spdlog::error("Error inserting session_resource: {}", e.what());
//         return -1;
//     }
// }

// // 插入 session_packets 表
// int MySQLDAO::insert_session_packet(const std::string& session_id, const json& packet) {
//     auto conn = m_pool->get_connection();
//     if (!conn) return -1;

//     try {
//         std::string sql = R"(
//             INSERT INTO session_packets (session_id, packet_json) VALUES (?, ?)
//         )";
//         std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
//         stmt->setString(1, session_id);
//         stmt->setString(2, packet.dump());
//         stmt->execute();

//         m_pool->return_connection(std::move(conn));
//         return 1;
//     } catch (const std::exception& e) {
//         m_pool->return_connection(std::move(conn));
//         spdlog::error("Error inserting session_packet: {}", e.what());
//         return -1;
//     }
// }