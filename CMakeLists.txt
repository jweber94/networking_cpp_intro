cmake_minimum_required(VERSION 2.8)

project(networking_cpp_intro)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

file(GLOB project_SRCS src/*.cpp)
file(GLOB project_INCL src/*.hpp)

add_executable(networking_cpp ${project_SRCS} ${project_INCL})
target_link_libraries(networking_cpp PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(networking_cpp PRIVATE ${Boost_INCLUDE_DIRS})

