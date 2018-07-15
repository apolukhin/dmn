project(DMN)
cmake_minimum_required(VERSION 2.8)
enable_testing()


# Boost setup
if (NOT BOOST_ROOT)
    set(BOOST_ROOT "/data/boost_1_67_0")
endif()
set(Boost_NO_SYSTEM_PATHS OFF)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS filesystem thread system graph regex unit_test_framework REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})
MESSAGE("Using Boost libraries path: ${Boost_LIBRARY_DIRS}")


# Project configuration
add_definitions(-DBOOST_THREAD_VERSION=4)
add_definitions(-DBOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS=1)
add_definitions(-DBOOST_ENABLE_ASSERT_DEBUG_HANDLER=1)
include_directories(${Boost_INCLUDE_DIRS} src)

if (MSVC)
else()
    add_compile_options(-std=c++14)
    add_compile_options(-fvisibility=hidden)
    add_compile_options(-Wno-deprecated-declarations)

    if (CMAKE_BUILD_TYPE MATCHES DEBUG)
        add_compile_definitions(-D_GLIBCXX_DEBUG_PEDANTIC)
    else()
        add_definitions(-DNDEBUG)

        # LTO flags
        add_compile_options(-flto)
        set(CMAKE_AR  "gcc-ar")
        set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_C_ARCHIVE_FINISH   true)
        set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_CXX_ARCHIVE_FINISH   true)
        set_property(DIRECTORY . PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    endif()

    # -DCMAKE_CXX_FLAGS_DEBUG="-fuse-ld=gold -fsanitize=address,undefined"
endif()


# Core library
add_library(dmn_core STATIC
    src/assert.cpp

    src/load_graph.cpp
    src/load_graph.hpp

    src/node_base.cpp
    src/node_base.hpp
    src/node.cpp
    src/node.hpp

    src/stream.hpp
    src/utility.hpp

    src/impl/circular_iterator.hpp
    src/impl/compare_addrs.hpp
    src/impl/lazy_array.hpp
    src/impl/packet.cpp
    src/impl/packet.hpp
    src/impl/saturation_timer.hpp
    src/impl/silent_mt_queue.hpp
    src/impl/state_tracker.hpp
    src/impl/work_counter.hpp

    src/impl/edges/edge_in.hpp
    src/impl/edges/edge_out.hpp

    src/impl/net/interval_timer.hpp
    src/impl/net/netlink.hpp
    src/impl/net/packet_network.cpp
    src/impl/net/packet_network.hpp
    src/impl/net/tcp_acceptor.hpp
    src/impl/net/tcp_read_proto.cpp
    src/impl/net/tcp_read_proto.hpp
    src/impl/net/tcp_write_proto.cpp
    src/impl/net/tcp_write_proto.hpp
    src/impl/net/proto_common.cpp
    src/impl/net/proto_common.hpp
    src/impl/net/slab_allocator.hpp
    src/impl/net/wrap_handler.hpp

    src/impl/node_parts/packets_gatherer.hpp
    src/impl/node_parts/read_0.hpp
    src/impl/node_parts/read_1.hpp
    src/impl/node_parts/read_n.hpp
    src/impl/node_parts/write_0.hpp
    src/impl/node_parts/write_1.hpp
    src/impl/node_parts/write_n.hpp
)
target_link_libraries(dmn_core PRIVATE ${Boost_SYSTEM_LIBRARY} ${Boost_GRAPH_LIBRARY} ${Boost_REGEX_LIBRARY} -ldl -lpthread )


# Main executable
add_executable(dmn src/main.cpp)

# Tests
aux_source_directory(tests SRC_LIST_TESTS)
aux_source_directory(tests/nodes SRC_LIST_TESTS_NODES)
add_executable(dmn_tests ${SRC_LIST_TESTS} ${SRC_LIST_TESTS_NODES} tests/tests_common.hpp)
target_link_libraries(dmn_tests dmn_core ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
