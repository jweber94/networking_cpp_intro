cmake_minimum_required(VERSION 3.19.8)

project(networking_cpp_intro)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Setup testing --> cmake must know if there is GTest installed on your machine. This MUST be in the top Level CMakeLists.txt to be recognized by IDEs and is valid in all subordinated CMakeLists.txt (e.g in /tests)
enable_testing() # tell cmake that you want it to generate test scripts --> "enable ctest"
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR}) # include the gtest includes globally. It is possible to include it just for a target with the variable 

# normal client
add_executable(http_client src/main_client.cpp)
target_link_libraries(http_client PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(http_client PRIVATE ${Boost_INCLUDE_DIRS})

# asynchrounous client
add_executable(async_http_client src/main_client_async.cpp)
target_link_libraries(async_http_client PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(async_http_client PRIVATE ${Boost_INCLUDE_DIRS})

# custom networking library
add_library(custom_net_lib INTERFACE) #/src/custom_net_lib/*.cpp /src/custom_net_lib/*.hpp)
target_include_directories(custom_net_lib INTERFACE include/custom_net_lib/ ${Boost_INCLUDE_DIRS})
target_link_libraries(custom_net_lib INTERFACE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Tests
add_subdirectory(tests)

