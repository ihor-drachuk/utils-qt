set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS QuickTest QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS QuickTest)
endif()

add_executable(test-utils-qt-4 main4.cpp MLMResetter.h main4.qrc)
set_property(TARGET test-utils-qt-4 PROPERTY CXX_STANDARD 17)
set_property(TARGET test-utils-qt-4 PROPERTY AUTOMOC ON)
set_property(TARGET test-utils-qt-4 PROPERTY AUTORCC ON)
target_link_libraries(test-utils-qt-4 PRIVATE Qt${QT_VERSION_MAJOR}::QuickTest utils-qt)
target_compile_definitions(test-utils-qt-4 PRIVATE QUICK_TEST_SOURCE_DIR=":/UtilsQt/QmlTests/" UTILS_QT_ROOT_DIR="${UTILS_QT_ROOT_DIR}")

target_compile_options(test-utils-qt-4 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

if (NOT UTILS_QT_NO_GUI_TESTS)
    add_test(NAME test-utils-qt-4-runner COMMAND test-utils-qt-4)
endif()
