set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test-utils-qt-3 main3.cpp main3.qrc)
set_property(TARGET test-utils-qt-3 PROPERTY CXX_STANDARD 17)
set_property(TARGET test-utils-qt-3 PROPERTY AUTOMOC ON)
set_property(TARGET test-utils-qt-3 PROPERTY AUTORCC ON)
target_link_libraries(test-utils-qt-3 PRIVATE utils-qt)

target_compile_options(test-utils-qt-3 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

#add_test(NAME test-utils-qt-3-runner COMMAND test-utils-qt-3)
