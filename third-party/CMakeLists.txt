add_subdirectory(muduo)
add_subdirectory(concurrentqueue)

add_library(third-party INTERFACE)

find_package(Threads REQUIRED)

# Protobuf
find_package(Protobuf REQUIRED)

target_include_directories(third-party INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/muduo
    ${CMAKE_CURRENT_SOURCE_DIR}/concurrentqueue
)

target_link_libraries(third-party INTERFACE
    muduo_base
    muduo_net
    concurrentqueue
    Threads::Threads
    protobuf::libprotobuf
)
