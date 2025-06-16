file(GLOB_RECURSE RAFT_CORE_SOURCES "*.cpp" "*.cc")

add_library(raft-core STATIC ${RAFT_CORE_SOURCES})

target_include_directories(raft-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raft-core PUBLIC
    WW::logger
)

add_library(WW::raft-core ALIAS raft-core)
