file(GLOB_RECURSE RAFT_SOURCES "*.cpp")

add_library(raft-app STATIC ${RAFT_SOURCES})

target_include_directories(raft-app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raft-app PUBLIC
    WW::kvstore
    WW::logger
    WW::raft-core
    WW::raft-rpc
    third-party
)

add_library(WW::raft-app ALIAS raft-app)
