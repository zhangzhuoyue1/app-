cmake_minimum_required(VERSION 3.15)
project(ProjectSuite LANGUAGES CXX)
option(BUILD_TESTING "Build the testing tree" ON)
enable_testing()
# -------------------------------- 全局配置 --------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出目录规范（保持build目录干净）
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 在项目级别查找jsoncpp
find_package(jsoncpp REQUIRED)

# 设置全局变量
set(JSONCPP_INCLUDE_DIR ${jsoncpp_INCLUDE_DIRS})
set(JSONCPP_LIBRARY ${jsoncpp_LIBRARIES})


# -------------------------------- 添加子目录 --------------------------------
add_subdirectory(message_parse)  # 消息解析库
add_subdirectory(mysql)          # 数据库库
add_subdirectory(zmq_server)     # 网络通信库
add_subdirectory(app)            # 主程序目录
add_subdirectory(test)           # 测试目录（最后添加）