aux_source_directory(./ xtransport_demo_dir)
add_executable(xtransport_demo ${xtransport_demo_dir})

add_dependencies(xtransport_demo xgossip xtransport xxbase)

target_link_libraries(xtransport_demo
    xtransport
    xpbase
    xcrypto
    xutility
    xxbase
    xledger
    protobuf
    -lgtest
    -lgmock
    -lpthread -ldl
    -lrt
)

