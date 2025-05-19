file(GLOB_RECURSE RPC_CORE_SOURCES ".cc" ".cpp")

add_library(rpc-core STATIC ${RPC_CORE_SOURCES})

target_include_directories(rpc-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/config/include
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/include
    ${CMAKE_CURRENT_SOURCE_DIR}/zookeeper/include
    ${CMAKE_CURRENT_SOURCE_DIR}/client/include
    ${CMAKE_CURRENT_SOURCE_DIR}/server/include
)

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# ZooKeeper
include_directories(/usr/include)
link_directories(/usr/lib)

target_link_libraries(rpc-core PRIVATE
    muduo_base
    muduo_net
    Threads::Threads
    protobuf::libprotobuf
    zookeeper_mt
)

add_library(WW::RPC-CORE ALIAS rpc-core)
