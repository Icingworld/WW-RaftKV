file(GLOB_RECURSE RAFT_EXAMPLE_SOURCES "*.cpp")

add_executable(raft_example ${RAFT_EXAMPLE_SOURCES})

target_link_libraries(raft_example PRIVATE
    WW::raft
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run.sh ${CMAKE_CURRENT_BINARY_DIR}/run.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stop.sh ${CMAKE_CURRENT_BINARY_DIR}/stop.sh COPYONLY)
