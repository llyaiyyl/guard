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
#target_link_libraries(app ${OpenCV_LIBRARIES})

set(ASIO_INCLUDE_DIR ~/bin/boost_1_68_0/include)
set(ASIO_LIB_DIR ~/bin/boost_1_68_0/lib)
set(ASIO_LIBRARIES libboost_system.a libboost_thread.a)

include_directories(${ASIO_INCLUDE_DIR})
link_directories(${ASIO_LIB_DIR})

aux_source_directory(./src/server SERVER_SRCS)
aux_source_directory(./src/client CLIENT_SRCS)

add_executable(guard-server ${SERVER_SRCS})
add_executable(guard-client ${CLIENT_SRCS})

target_link_libraries(guard-server ${ASIO_LIBRARIES} pthread)
target_link_libraries(guard-client ${ASIO_LIBRARIES} pthread)
# target_include_directories(guard-server PRIVATE ${ASIO_INCLUDE})
# target_include_directories(guard-client PRIVATE ${ASIO_INCLUDE})


