set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test-utils-qt-5 main5.cpp main5.qrc)
set_property(TARGET test-utils-qt-5 PROPERTY CXX_STANDARD 17)
set_property(TARGET test-utils-qt-5 PROPERTY AUTOMOC ON)
set_property(TARGET test-utils-qt-5 PROPERTY AUTORCC ON)
target_link_libraries(test-utils-qt-5 PRIVATE utils-qt)

target_compile_options(test-utils-qt-5 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

if (NOT UTILS_QT_NO_GUI_TESTS)
    add_test(NAME test-utils-qt-5-runner COMMAND test-utils-qt-5)
endif()
