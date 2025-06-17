file(GLOB_RECURSE HEADER_FILES include/*.h)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# 显式指定源文件（避免自动查找可能遗漏）
set(SRC_FILES
    src/main.cpp
    src/HttpServer.cpp
    src/HttpConnection.cpp
    src/LogicSystem.cpp
    src/const.cpp
)



add_executable(app  ${SRC_FILES}  ${HEADER_FILES})


# 链接所有需要的子库
target_link_libraries(app PUBLIC
    message_parse 
    mysql 
    Boost::boost       # 头文件
    Boost::system      # Boost系统库
    zmq_server
    Threads::Threads
)

# 自动继承所有子库的头文件路径（无需手动指定）