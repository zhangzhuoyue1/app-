cmake_minimum_required(VERSION 3.0)
project(Server)
find_package(spdlog REQUIRED)  # 自动查找系统安装的spdlog

include_directories(./include)
aux_source_directory(src DIR_SRCS)
add_executable(Server ${DIR_SRCS})

