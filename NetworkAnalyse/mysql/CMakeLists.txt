# 定义模块化库
file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE HEADER_FILES include/*.h)

add_library(mysql STATIC  # 静态库
    ${SRC_FILES}
    ${HEADER_FILES}
)

# 暴露公共头文件路径（自动传递到主程序）
target_include_directories(mysql PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 添加私有依赖（如该库需要第三方组件）
target_link_libraries(mysql mysqlcppconn)

