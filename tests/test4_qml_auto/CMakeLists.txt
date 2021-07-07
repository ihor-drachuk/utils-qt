cmake_minimum_required(VERSION 3.16)

project(test-utils-qt-4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS QuickTest REQUIRED)

add_executable(test-utils-qt-4 main4.cpp main4.qrc)
set_property(TARGET test-utils-qt-4 PROPERTY CXX_STANDARD 17)
set_property(TARGET test-utils-qt-4 PROPERTY AUTOMOC ON)
set_property(TARGET test-utils-qt-4 PROPERTY AUTORCC ON)
target_link_libraries(test-utils-qt-4 PRIVATE Qt5::QuickTest utils-qt)
target_compile_definitions(test-utils-qt-4 PRIVATE QUICK_TEST_SOURCE_DIR=":/UtilsQt/QmlTests/")

target_compile_options(test-utils-qt-4 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

if (UTILS_QT_ALLOW_QML_TESTS)
    add_test(NAME test-utils-qt-4-runner COMMAND test-utils-qt-4)
endif()
