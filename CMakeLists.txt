project(DMN)
cmake_minimum_required(VERSION 2.8)

set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# Boost setup
set(BOOST_ROOT "/data/boost")
add_definitions(-DBOOST_TEST_DYN_LINK)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS system filesystem thread unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} src)
link_directories(${Boost_LIBRARY_DIRS})
MESSAGE("Using Boost libraries path: ${Boost_LIBRARY_DIRS}")


add_library(dmn_core STATIC
    src/load_graph.hpp
    src/load_graph.cpp
    src/node.hpp
    src/node.cpp
)


add_executable(dmn src/main.cpp)


aux_source_directory(tests SRC_LIST_TESTS)
add_executable(dmn_tests ${SRC_LIST_TESTS})
target_link_libraries(dmn_tests dmn_core ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
