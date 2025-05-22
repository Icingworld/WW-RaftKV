file(GLOB_RECURSE LOGGER_SOURCES "*.cpp")

add_library(logger STATIC ${LOGGER_SOURCES})

target_include_directories(logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/formatter/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sink/include
    ${CMAKE_CURRENT_SOURCE_DIR}/logger/include
)

find_package(Threads REQUIRED)

target_link_libraries(logger PRIVATE
    Threads::Threads
)

add_library(WW::logger ALIAS logger)
