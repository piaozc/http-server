cmake_minimum_required(VERSION 3.16)

project(cloud_disk_network_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(my_server
    src/main.cpp
    src/net/ServerSocket.cpp
    src/reactor/MainReactor.cpp
    src/reactor/SubReactor.cpp
    src/threadpool/ThreadPool.cpp
    src/http/HttpParser.cpp
    src/http/HttpResponse.cpp
    src/business/MockBusinessClient.cpp
    src/connection/Connection.cpp
)

target_include_directories(my_server PRIVATE src)
find_package(Threads REQUIRED)
target_link_libraries(my_server PRIVATE Threads::Threads)
