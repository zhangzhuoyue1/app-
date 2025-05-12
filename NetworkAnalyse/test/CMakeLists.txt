cmake_minimum_required(VERSION 3.15)
project(network_analyzer_test)

# 启用测试框架
enable_testing()

# 查找依赖包
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# 添加测试可执行文件
add_executable(network_analyzer_test 
    test.cpp
    # 可添加其他测试源文件
)

# 包含路径配置（严格匹配项目结构）
target_include_directories(network_analyzer_test PRIVATE
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/zmq_server/include

)

# 链接依赖库（注意顺序）
target_link_libraries(network_analyzer_test PRIVATE
    GTest::GTest
    GTest::Main
    zmq_server
    ${CMAKE_THREAD_LIBS_INIT}
)


# 注册测试用例
add_test(NAME network_analyzer_tests  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/network_analyzer_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})