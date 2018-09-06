cmake_minimum_required(VERSION 2.8)

# project name
project(guard)

# include head file
include_directories(~/bin/ffmpeg-4.0.2/include)
include_directories(~/bin/zlib-1.2.11/include)

# lib dir
link_directories(~/bin/ffmpeg-4.0.2/lib)
link_directories(~/bin/zlib-1.2.11/lib)

add_executable(guard-server src/server.c)
add_executable(guard-client src/client.c)

target_link_libraries(guard-server pthread)
target_link_libraries(guard-client libavfilter.a libavformat.a libavdevice.a libavcodec.a libswscale.a libavutil.a libswresample.a libz.a m pthread)
