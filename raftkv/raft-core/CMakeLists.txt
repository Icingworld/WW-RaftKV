file(GLOB_RECURSE RAFT_CORE_SOURCES "*.cpp" "*.cc")

add_library(raft-core STATIC ${RAFT_CORE_SOURCES})

target_include_directories(raft-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc/message/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc/client/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc/server/include
    ${CMAKE_CURRENT_SOURCE_DIR}/raft/include
)

target_link_libraries(raft-core PUBLIC
    third-party
    WW::rpc-core
)

add_library(WW::raft-core ALIAS raft-core)
