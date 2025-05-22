file(GLOB_RECURSE RAFT_EXAMPLE_SOURCES "*.cpp")

add_executable(raft_example ${RAFT_EXAMPLE_SOURCES})

target_link_libraries(raft_example PRIVATE
    WW::raft-core
)
