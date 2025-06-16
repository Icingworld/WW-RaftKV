file(GLOB_RECURSE RAFT_SOURCES "*.cpp" "*.cc")

add_library(raft-rpc STATIC ${RAFT_SOURCES})

target_include_directories(raft-rpc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raft-rpc PUBLIC
    WW::logger
    third-party
)

add_library(WW::raft-rpc ALIAS raft-rpc)
