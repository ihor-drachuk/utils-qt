cmake_minimum_required(VERSION 3.16)
PROJECT(utils-qt)

option(UTILS_QT_ENABLE_TESTS     "utils-qt: Enable tests" OFF)
option(UTILS_QT_ENABLE_BENCHMARK "utils-qt: Enable benchmark" OFF)

if (NOT TARGET utils-cpp)
    add_subdirectory(3rd-party/utils-cpp)
endif()

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.h headers/*.h)

add_library(utils-qt STATIC ${SOURCES})
set_property(TARGET utils-qt PROPERTY CXX_STANDARD 17)
set_property(TARGET utils-qt PROPERTY AUTOMOC ON)

target_compile_definitions(utils-qt PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(utils-qt PUBLIC Qt5::Core Qt5::Quick utils-cpp)
target_include_directories(utils-qt PUBLIC headers)

if (UTILS_QT_CI_RUN)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_CI_RUN)
endif()

add_subdirectory(tests)
