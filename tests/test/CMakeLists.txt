find_package(Qt6 COMPONENTS Test)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Test)
endif()

FILE(GLOB_RECURSE SOURCES *.cpp)

set(PROJECT_TEST_NAME test-${PROJECT_NAME})

add_executable(${PROJECT_TEST_NAME} ${SOURCES})
set_property(TARGET ${PROJECT_TEST_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_TEST_NAME} PROPERTY AUTOMOC ON)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_TEST_NAME} gtest gmock_main Threads::Threads Qt${QT_VERSION_MAJOR}::Test ${PROJECT_NAME})

add_test(NAME ${PROJECT_TEST_NAME}-runner COMMAND ${PROJECT_TEST_NAME})

if(MSVC)
    target_link_options(${PROJECT_TEST_NAME} PRIVATE "/ignore:4221")
    set_target_properties(${PROJECT_TEST_NAME} PROPERTIES STATIC_LIBRARY_OPTIONS "/ignore:4221")
    target_compile_options(${PROJECT_TEST_NAME} PRIVATE "/WX")
else()
    target_compile_options(${PROJECT_TEST_NAME} PRIVATE "-Werror")
endif()
