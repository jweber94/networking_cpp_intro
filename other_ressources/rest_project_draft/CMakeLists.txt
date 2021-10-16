cmake_minimum_required(VERSION 3.11.4)

project(rest_json_server)

find_package(Boost COMPONENTS program_options filesystem REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_executable(rest_server src/main.cpp src/networking_logic.cpp src/server_logic.cpp)
target_link_libraries(rest_server PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(rest_server PRIVATE ${Boost_INCLUDE_DIRS} include/rest_project_draft)