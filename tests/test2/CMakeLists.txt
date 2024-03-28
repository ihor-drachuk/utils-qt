set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test-utils-qt-2 main2.cpp)
set_property(TARGET test-utils-qt-2 PROPERTY CXX_STANDARD 17)
target_link_libraries(test-utils-qt-2 PRIVATE utils-qt)

target_compile_options(test-utils-qt-2 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

add_test(NAME test-utils-qt-2-runner COMMAND test-utils-qt-2)
