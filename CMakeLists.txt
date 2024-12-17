cmake_minimum_required(VERSION 3.16)
PROJECT(utils-qt LANGUAGES CXX)

option(UTILS_QT_ENABLE_TESTS     "utils-qt: Enable tests" OFF)
option(UTILS_QT_ENABLE_BENCHMARK "utils-qt: Enable benchmark" OFF)
option(UTILS_QT_NO_GUI_TESTS     "utils-qt: Disable GUI tests" OFF)
option(UTILS_QT_LONG_INTERVALS   "utils-qt: Enable long intervals in tests" OFF)

set(UTILS_QT_GTEST_SEARCH_MODE "Auto" CACHE STRING "utils-qt: Set GTest search mode")
set_property(CACHE UTILS_QT_GTEST_SEARCH_MODE PROPERTY STRINGS "Auto" "Force" "Skip")

set(UTILS_QT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT TARGET utils-cpp)
    add_subdirectory(3rd-party/utils-cpp)
endif()

find_package(Qt6 COMPONENTS Core Quick QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Quick)
endif()

FILE(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.qrc src/*.h headers/*.h)

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
    target_compile_options(utils-qt PRIVATE "/WX")

    # MSVC 2022 + Qt5 + QVector::operator== warning mitigation
    if(Qt5_FOUND AND MSVC_VERSION GREATER_EQUAL 1929)
        target_compile_definitions(utils-qt PUBLIC _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1 _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING=1)
    endif()
else()
    target_compile_options(utils-qt PRIVATE "-Werror")
endif()

# --- System detection ---
if (WIN32)
    set(UTILS_QT_OS_WIN TRUE)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_OS_WIN=1)
endif()

if(UNIX AND NOT APPLE)
    set(UTILS_QT_OS_LINUX TRUE)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_OS_LINUX=1)
endif()

if(APPLE)
    set(UTILS_QT_OS_MACOS TRUE)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_OS_MACOS=1)
endif()

if (MSVC)
    set(UTILS_QT_COMPILER_MSVC TRUE)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_COMPILER_MSVC=1)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(UTILS_QT_COMPILER_GCC TRUE)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_COMPILER_GCC=1)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(UTILS_QT_COMPILER_CLANG TRUE)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_COMPILER_CLANG=1)
endif()
# --- ---

target_link_libraries(utils-qt PUBLIC Qt::Core Qt::Quick utils-cpp)
target_include_directories(utils-qt PUBLIC headers)

if (UTILS_QT_NO_GUI_TESTS)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_NO_GUI_TESTS)
endif()

if (UTILS_QT_LONG_INTERVALS)
    target_compile_definitions(utils-qt PUBLIC UTILS_QT_LONG_INTERVALS)
endif()

add_subdirectory(tests)
