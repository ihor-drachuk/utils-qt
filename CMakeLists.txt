cmake_minimum_required(VERSION 3.16)
PROJECT(utils-qt)

option(UTILS_QT_ENABLE_TESTS     "utils-qt: Enable tests" OFF)
option(UTILS_QT_ENABLE_BENCHMARK "utils-qt: Enable benchmark" OFF)
option(UTILS_QT_NO_GUI_TESTS     "utils-qt: Disable GUI tests" OFF)

set(UTILS_QT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT TARGET utils-cpp)
    add_subdirectory(3rd-party/utils-cpp)
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick)

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

# --- System detection ---
if (WIN32)
    set(UTILS_QT_OS_WIN TRUE)
endif()

if(UNIX AND NOT APPLE)
    set(UTILS_QT_OS_LINUX TRUE)
endif()

if(APPLE)
    set(UTILS_QT_OS_MACOS TRUE)
endif()

if (MSVC)
    set(UTILS_QT_COMPILER_MSVC TRUE)
endif()

if (UTILS_QT_OS_WIN)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_OS_WIN=1)
endif()

if (UTILS_QT_OS_MACOS)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_OS_MACOS=1)
endif()

if (UTILS_QT_OS_LINUX)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_OS_LINUX=1)
endif()
# --- ---

target_link_libraries(utils-qt PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick utils-cpp)
target_include_directories(utils-qt PUBLIC headers)

if (UTILS_QT_NO_GUI_TESTS)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_NO_GUI_TESTS)
endif()

add_subdirectory(tests)
