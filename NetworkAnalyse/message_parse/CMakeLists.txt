# 定义模块化库
file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE HEADER_FILES include/*.h)

add_library(message_parse STATIC  # 静态库
    ${SRC_FILES}
    ${HEADER_FILES}
)

# 暴露公共头文件路径（自动传递到主程序）
target_include_directories(message_parse PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(message_parse PUBLIC
    mysql
    zmq_server
)
# 添加私有依赖（如该库需要第三方组件）
find_package(spdlog REQUIRED)  # 日志库
target_link_libraries(message_parse PRIVATE spdlog::spdlog)

find_path(PCAP_INCLUDE_DIR pcap.h)
find_library(PCAP_LIBRARY pcap)
include_directories(${PCAP_INCLUDE_DIR})
target_link_libraries(message_parse PRIVATE ${PCAP_LIBRARY})
