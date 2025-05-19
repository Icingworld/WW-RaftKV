add_library(kvstore INTERFACE)

target_include_directories(kvstore INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(WW::kvstore ALIAS kvstore)
