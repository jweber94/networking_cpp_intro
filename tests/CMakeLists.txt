cmake_minimum_required(VERSION 3.11.4)
project(test_custom_networking)

# this is the cmake that describes the tests

set(test_sources test_serialization.cpp)
set(CMAKE_CXX_STANDARD 11) # This is very important for GTest to run!

# Setup testing --> cmake must know if there is GTest installed on your machine
#enable_testing() # tell cmake that you want it to generate test scripts
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIR}) # include the gtest includes globally. It is possible to include it just for a target with the variable 
# --> This MUST be in the Top-Level CMake to let ctest run with VSCode

find_package(Boost REQUIRED)

# create an executable that describes the tests
add_executable(unit_tests test_serialization.cpp)

# link gtest against the executable that contains the tests
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/custom_net_lib ${Boost_INCLUDE_DIRS})
target_link_libraries(unit_tests PUBLIC custom_net_lib gtest gtest_main pthread ${Boost_LIBRARIES}) # CAUTION: gtest requires pthread in the linked libraries

# define for cmake that this is a test
add_test(NAME unit_tests COMMAND unit_tests)
