# 创建动态库（或 STATIC 静态库）
file(GLOB_RECURSE MSG_PARSER_SOURCES "src/*.cpp")
file(GLOB_RECURSE MSG_PARSER_HEADERS "include/*.h")

add_library(database STATIC  ${MSG_PARSER_SOURCES} ${MSG_PARSER_HEADERS})

# 包含头文件目录
target_include_directories(database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# ----------------------------------依赖第三方库----------------------------------#
# sqdlog 日志库
find_package(spdlog REQUIRED)
target_link_libraries(database PUBLIC spdlog::spdlog)
# mysqlclient 数据库连接库
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
target_include_directories(database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MYSQL_INCLUDE_DIRS}
)
target_link_libraries(database PRIVATE
    ${MYSQL_LIBRARIES}
)
#redis 数据库
find_package(PkgConfig REQUIRED)
pkg_check_modules(REDIS REQUIRED hiredis)
target_include_directories(database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REDI_INCLUDE_DIRS}
)
target_link_libraries(database PRIVATE
    ${REDI_LIBRARIES}
)

# 安装配置
install(TARGETS database
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/database
)