file(GLOB_RECURSE RAFT_SOURCES "*.cpp" "*.cc")

add_library(raft STATIC ${RAFT_SOURCES})

target_include_directories(raft PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raft PUBLIC
    WW::raft-core
    WW::rpc-core
    WW::kvstore
    third-party
)

add_library(WW::raft ALIAS raft)
