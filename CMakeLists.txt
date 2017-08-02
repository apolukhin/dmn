project(DMN)
cmake_minimum_required(VERSION 2.8)

set_property(GLOBAL PROPERTY CXX_STANDARD 11)

# Boost libs setup
set(BOOST_ROOT "/data/boost")
add_definitions(-DBOOST_TEST_DYN_LINK)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS system filesystem thread unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
MESSAGE("Using Boost libraries path: ${Boost_LIBRARY_DIRS}")

aux_source_directory(src SRC_LIST)
add_executable(dmn ${SRC_LIST})

aux_source_directory(tests SRC_LIST_TESTS)
add_executable(dmn_tests ${SRC_LIST_TESTS})
target_link_libraries(dmn_tests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
