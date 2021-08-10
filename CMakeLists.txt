cmake_minimum_required(VERSION 2.8)

project(networking_cpp_intro)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# normal client
add_executable(http_client src/main_client.cpp)
target_link_libraries(http_client PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(http_client PRIVATE ${Boost_INCLUDE_DIRS})

# asynchrounous client
add_executable(async_http_client src/main_client_async.cpp)
target_link_libraries(async_http_client PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(async_http_client PRIVATE ${Boost_INCLUDE_DIRS})
