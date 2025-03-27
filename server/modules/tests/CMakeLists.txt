# 创建测试可执行文件
add_executable(test_parser test.cpp)

# 链接被测库及依赖项
target_link_libraries(test_parser 
    PRIVATE 
    message_parser          # 被测静态库
    gtest                   # Google Test主库
    GTest::gtest_main       #    Google Test主入口
)

# 自动发现并注册所有GTest用例
include(GoogleTest)
gtest_discover_tests(test_parser)

add_test(NAME test_parser COMMAND test_parser)