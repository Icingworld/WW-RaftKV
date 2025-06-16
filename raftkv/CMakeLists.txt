add_subdirectory(logger)
add_subdirectory(memory-pool)
add_subdirectory(kvstore)
add_subdirectory(raft-core)
add_subdirectory(raft-rpc)
add_subdirectory(raft-app)

add_library(raftkv INTERFACE)

target_include_directories(raftkv INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/logger
    ${CMAKE_CURRENT_SOURCE_DIR}/memory-pool
    ${CMAKE_CURRENT_SOURCE_DIR}/kvstore
    ${CMAKE_CURRENT_SOURCE_DIR}/raft-core
    ${CMAKE_CURRENT_SOURCE_DIR}/raft-rpc
    ${CMAKE_CURRENT_SOURCE_DIR}/raft-app
)

target_link_libraries(raftkv INTERFACE
    WW::raft-app
)

add_library(WW::raftkv ALIAS raftkv)
