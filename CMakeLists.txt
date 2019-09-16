cmake_minimum_required(VERSION 3.8)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/proto)

aux_source_directory(./src xtransport_src)
aux_source_directory(./proto proto_src)
add_library(xtransport ${xtransport_src} ${proto_src})

add_dependencies(xtransport xpbase xutility xxbase)
target_link_libraries(xtransport xpbase xutility xxbase protobuf)

if (BUILD_METRICS)
    add_dependencies(xtransport xmetrics)
    target_link_libraries(xtransport xmetrics)
endif()

if(XENABLE_TESTS)
    add_subdirectory(tests)
endif()
