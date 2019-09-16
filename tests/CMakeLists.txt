
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    aux_source_directory(./ xtransport_test_dir)
    add_executable(xtransport_test ${xtransport_test_dir})
    add_dependencies(xtransport_test xtransport xxbase)
    target_link_libraries(xtransport_test xtransport xxbase gtest gmock)

    if(XENABLE_CODE_COVERAGE)
        target_link_libraries(xtransport_test gcov)
    endif()
endif()
