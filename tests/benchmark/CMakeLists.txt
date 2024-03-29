file(GLOB SOURCES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

foreach( testsourcefile ${SOURCES} )
    string( REPLACE ".cpp" "" testname ${testsourcefile} )

    add_executable( benchmark-${testname} ${testsourcefile} )
    set_property(TARGET benchmark-${testname} PROPERTY CXX_STANDARD 17)

    find_package(Threads REQUIRED)
    target_link_libraries(benchmark-${testname} gtest benchmark Threads::Threads ${PROJECT_NAME})

    add_test(NAME benchmark-${testname}-runner COMMAND benchmark-${testname})

    if(MSVC)
        target_link_options(benchmark-${testname} PRIVATE "/ignore:4221")
        set_target_properties(benchmark-${testname} PROPERTIES STATIC_LIBRARY_OPTIONS "/ignore:4221")
        target_compile_options(benchmark-${testname} PRIVATE "/WX")
    else()
        target_compile_options(benchmark-${testname} PRIVATE "-Werror")
    endif()
endforeach( testsourcefile ${APP_SOURCES} )
