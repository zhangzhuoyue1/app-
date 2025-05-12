# 定义模块化库
file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE HEADER_FILES include/*.h)

add_library(zmq_server STATIC  # 静态库
    ${SRC_FILES}
    ${HEADER_FILES}
)

# 暴露公共头文件路径（自动传递到主程序）
target_include_directories(zmq_server PUBLIC 
    ${JSONCPP_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 添加私有依赖（如该库需要第三方组件）
find_package(spdlog REQUIRED)  # 日志库
target_link_libraries(message_parse PRIVATE spdlog::spdlog ${JSONCPP_LIBRARY})

#手动链接zmq库
find_path(ZMQ_INCLUDE_DIR zmq.h)
find_library(ZMQ_LIBRARY zmq)

if (NOT ZMQ_INCLUDE_DIR OR NOT ZMQ_LIBRARY)
    message(FATAL_ERROR "ZeroMQ not found")
endif()
# 添加头文件目录
target_include_directories(zmq_server PRIVATE ${ZMQ_INCLUDE_DIR})
# 链接库
target_link_libraries(zmq_server PRIVATE ${ZMQ_LIBRARY})

