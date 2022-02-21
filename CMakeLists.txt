cmake_minimum_required(VERSION 3.16)
PROJECT(utils-qt)

option(UTILS_QT_ENABLE_TESTS     "utils-qt: Enable tests" OFF)
option(UTILS_QT_SKIP_QML_TESTS   "utils-qt: Allow QML tests" OFF)
option(UTILS_QT_ENABLE_BENCHMARK "utils-qt: Enable benchmark" OFF)

set(UTILS_QT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT TARGET utils-cpp)
    add_subdirectory(3rd-party/utils-cpp)
endif()

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.qrc src/*.h headers/*.h)

list(APPEND QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/")
list(REMOVE_DUPLICATES QML_IMPORT_PATH)
set(QML_IMPORT_PATH "${QML_IMPORT_PATH}" CACHE STRING "Qt Creator extra qml import paths" FORCE)

add_library(utils-qt STATIC ${SOURCES})
target_compile_features(utils-qt PUBLIC cxx_std_17)
set_property(TARGET utils-qt PROPERTY AUTOMOC ON)
set_property(TARGET utils-qt PROPERTY AUTORCC ON)

if(MSVC)
    target_link_options(utils-qt PRIVATE "/ignore:4221")
    set_target_properties(utils-qt PROPERTIES STATIC_LIBRARY_OPTIONS "/ignore:4221")
endif()

target_link_libraries(utils-qt PUBLIC Qt5::Core Qt5::Quick utils-cpp)
target_include_directories(utils-qt PUBLIC headers)

if (UTILS_QT_CI_RUN)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_CI_RUN)
endif()

add_subdirectory(tests)
