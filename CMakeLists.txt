cmake_minimum_required(VERSION 2.8)

# project name
project(guard)

# include head file
#include_directories(~/bin/ffmpeg-4.0.2/include)
#include_directories(~/bin/zlib-1.2.11/include)
#include_directories(./list/)
#include_directories(~/bin/jrtplib-3.11.1/include/jrtplib3)
#include_directories(~/bin/jthread-1.3.3/include)
#include_directories(~/bin/opencv-3.4.3/include)

# lib dir
#link_directories(~/bin/ffmpeg-4.0.2/lib)
#link_directories(~/bin/zlib-1.2.11/lib)
#link_directories(~/bin/jrtplib-3.11.1/lib)
#link_directories(~/bin/jthread-1.3.3/lib)
#link_directories(~/bin/opencv-3.4.3/lib)

#aux_source_directory(./list/ LIST_SRCS)

#set(OpenCV_DIR "~/bin/opencv-3.4.3/share/OpenCV")
#find_package(OpenCV)

#add_executable(guard-server src/server.cpp ${LIST_SRCS})
#add_executable(guard-client src/client.cpp ${LIST_SRCS})
#add_executable(app src/app.cpp)

#target_link_libraries(guard-server libjrtp.a libjthread.a pthread)
#target_link_libraries(guard-client libjrtp.a libjthread.a libavfilter.a libavformat.a libavdevice.a libavcodec.a libswscale.a libavutil.a libswresample.a libz.a m pthread)

# include head file
include_directories(~/bin/jrtplib-3.11.1/include/jrtplib3)
include_directories(~/bin/jthread-1.3.3/include)
include_directories(~/bin/jsoncpp-1.8.4/include)
include_directories(~/workspace/coco/src)
include_directories(./src/common)

# lib dir
link_directories(~/bin/jrtplib-3.11.1/lib)
link_directories(~/bin/jthread-1.3.3/lib)
link_directories(~/bin/jsoncpp-1.8.4/lib)
link_directories(~/workspace/coco/build)

aux_source_directory(./src/common COMMON_SRCS)
aux_source_directory(./src/server SERVER_SRCS)
aux_source_directory(./src/client CLIENT_SRCS)
aux_source_directory(./src/manage MANAGE_SRCS)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(guard-server ${SERVER_SRCS} ${COMMON_SRCS})
add_executable(guard-client ${CLIENT_SRCS} ${COMMON_SRCS})
add_executable(guard-manage ${MANAGE_SRCS} ${COMMON_SRCS})

target_link_libraries(guard-server libjrtp.a libjthread.a libcoco.a libjsoncpp.a pthread)
target_link_libraries(guard-client libjrtp.a libjthread.a libcoco.a libjsoncpp.a pthread)
target_link_libraries(guard-manage libjrtp.a libjthread.a libcoco.a libjsoncpp.a pthread)


















