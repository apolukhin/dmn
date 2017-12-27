project(DMN)
cmake_minimum_required(VERSION 2.8)

if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_CXX_FLAGS "-std=c++14 ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# Boost setup
set(BOOST_ROOT "/data/boost")
add_definitions(-DBOOST_TEST_DYN_LINK)
add_definitions(-DBOOST_THREAD_VERSION=4)
add_definitions(-DBOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS=1)
add_definitions(-DBOOST_ENABLE_ASSERT_DEBUG_HANDLER=1)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS filesystem thread unit_test_framework system REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} src)
link_directories(${Boost_LIBRARY_DIRS})
MESSAGE("Using Boost libraries path: ${Boost_LIBRARY_DIRS}")


add_library(dmn_core STATIC

    src/assert.cpp
    src/utility.hpp
    src/silent_mt_queue.hpp
    src/work_counter.hpp

    src/slab_allocator.hpp
    src/wrap_handler.hpp

    src/load_graph.hpp
    src/load_graph.cpp

    src/netlink.hpp
    src/netlink.cpp
    src/packet.hpp
    src/packet.cpp
    src/packet_network.hpp
    src/packet_network.cpp
    src/stream.hpp

    src/node.hpp
    src/node.cpp
    src/node_base.hpp
    src/node_base.cpp
    src/state_tracker.hpp

    src/node_impl/read_0.hpp
    src/node_impl/read_1.hpp
    src/node_impl/write_0.hpp
    src/node_impl/write_1.hpp
)


add_executable(dmn src/main.cpp)


aux_source_directory(tests SRC_LIST_TESTS)
add_executable(dmn_tests ${SRC_LIST_TESTS} tests/tests_common.hpp)
target_link_libraries(dmn_tests dmn_core boost_system -ldl ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
