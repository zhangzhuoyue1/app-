# 创建动态库（或 STATIC 静态库）
file(GLOB_RECURSE MSG_PARSER_SOURCES "src/*.cpp")
file(GLOB_RECURSE MSG_PARSER_HEADERS "include/*.h")

add_library(message_parser STATIC  ${MSG_PARSER_SOURCES} ${MSG_PARSER_HEADERS})

# 包含头文件目录
target_include_directories(message_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 依赖第三方库
find_package(spdlog REQUIRED)
target_link_libraries(message_parser PUBLIC spdlog::spdlog)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)
target_include_directories(message_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PCAP_INCLUDE_DIRS}
)
target_link_libraries(message_parser PRIVATE
    ${PCAP_LIBRARIES}
)

# 安装配置
install(TARGETS message_parser
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/message_parser
)