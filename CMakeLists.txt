cmake_minimum_required(VERSION 3.28)
project(deep_into_rocksdb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(RocksDB REQUIRED)

add_executable(deep_into_rocksdb main.cpp)
add_executable(cached_example cached_example.cpp)
add_executable(custom_memory_allocator custom_memory_allocator.cpp)

# Link RocksDB
target_link_libraries(deep_into_rocksdb PRIVATE RocksDB::rocksdb)
target_link_libraries(cached_example PRIVATE RocksDB::rocksdb)
target_link_libraries(custom_memory_allocator PRIVATE RocksDB::rocksdb)

# Include directories
target_include_directories(deep_into_rocksdb PRIVATE ${ROCKSDB_INCLUDE_DIRS})
target_include_directories(cached_example PRIVATE ${ROCKSDB_INCLUDE_DIRS})
target_include_directories(custom_memory_allocator PRIVATE ${ROCKSDB_INCLUDE_DIRS})

target_compile_options(deep_into_rocksdb PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(cached_example PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(custom_memory_allocator PRIVATE -Wall -Wextra -Wpedantic)



