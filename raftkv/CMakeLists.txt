add_subdirectory(logger)
add_subdirectory(memory-pool)
add_subdirectory(kvstore)
add_subdirectory(rpc-core)
add_subdirectory(raft-core)
add_subdirectory(raft)

add_library(raftkv INTERFACE)

target_include_directories(raftkv INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/logger
    ${CMAKE_CURRENT_SOURCE_DIR}/memory-pool
    ${CMAKE_CURRENT_SOURCE_DIR}/kvstore
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc-core
    ${CMAKE_CURRENT_SOURCE_DIR}/raft-core
)

target_link_libraries(raftkv INTERFACE
    WW::raft
)

add_library(WW::raftkv ALIAS raftkv)
