cmake_minimum_required(VERSION 3.27)
project(BlockCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_SOURCES
    app/block_cache.cpp
    app/search_name.cpp
    app/old_search_name.cpp
)

set(BENCH_SOURCES
    app/main.cpp
)

set(TEST_SOURCES
    test/tests.cpp
)

include_directories(include)

add_library(block_cache SHARED ${LIB_SOURCES})
target_include_directories(block_cache PUBLIC include)

add_executable(main ${BENCH_SOURCES})
target_link_libraries(main PRIVATE block_cache)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE block_cache GTest::GTest GTest::Main pthread)
add_test(NAME Tests COMMAND tests)