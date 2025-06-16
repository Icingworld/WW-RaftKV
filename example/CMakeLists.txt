# raft_example
set(RAFT_EXAMPLE_SOURCES
    src/raft_example.cpp
)

add_executable(raft_example ${RAFT_EXAMPLE_SOURCES})

target_include_directories(raft_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raft_example PRIVATE
    WW::raftkv
)

# raft_client
set(CLIENT_SOURCES
    src/client_channel.cpp
    src/raft_client.cpp
    src/KVOperation.pb.cc
    src/uuid.cpp
)

add_executable(raft_client ${CLIENT_SOURCES})

target_include_directories(raft_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raft_client PRIVATE
    WW::raftkv
    third-party
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run.sh ${CMAKE_CURRENT_BINARY_DIR}/run.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stop.sh ${CMAKE_CURRENT_BINARY_DIR}/stop.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clean.sh ${CMAKE_CURRENT_BINARY_DIR}/clean.sh COPYONLY)
