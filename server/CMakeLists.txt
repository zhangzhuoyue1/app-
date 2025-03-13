cmake_minimum_required(VERSION 3.0)

project(Server)

# ----------- 编译选项 -----------
set(CMAKE_BUILD_TYPE Debug)

# ----------- 全局配置 -----------
set(CMAKE_CXX_STANDARD 17)      # C++ 标准

# ----------- 第三方库 -----------
find_package(spdlog REQUIRED)  # 自动查找系统安装的spdlog
find_library(PCAP_LIBRARY pcap) # 查找 libpcap 库
find_path(PCAP_INCLUDE_DIR pcap.h) # 查找 pcap.h 头文件

#----------- 子目录 -----------
add_subdirectory(src)

