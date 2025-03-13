# ----------------- 定义静态库 AppInfoFetcher -----------------
add_library(AppInfoFetcher STATIC 
    AppInfoFetcher/AppInfoFetcher.cpp    # 源文件
    AppInfoFetcher/AppInfoFetcher.h      # 头文件
)
target_link_libraries(AppInfoFetcher PRIVATE spdlog::spdlog)

# ----------------- 定义静态库 TrafficCapture -----------------
add_library(TrafficCapture STATIC 
    TrafficCapture/TrafficCapture.cpp     # 源文件
    TrafficCapture/TrafficCapture.h       # 修正后的头文件路径
)
target_link_libraries(TrafficCapture PRIVATE 
    spdlog::spdlog
    ${PCAP_LIBRARY}
    ${PCAP_INCLUDE_DIR}
    ) 

# ----------------- 定义可执行文件 your_main -----------------
add_executable(your_main
    main.cpp
)

# 链接静态库
target_link_libraries(your_main PRIVATE
    AppInfoFetcher
    TrafficCapture
)