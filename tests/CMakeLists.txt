# Fix warning for FetchContent_Declare regarding DOWNLOAD_EXTRACT_TIMESTAMP
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

set(UTILS_QT_NEED_GTEST OFF)
set(UTILS_QT_NEED_BENCHMARK OFF)

if (NOT UTILS_QT_ENABLE_TESTS AND NOT UTILS_QT_ENABLE_BENCHMARK)
    return()
endif()

include(FetchContent)

if (UTILS_QT_ENABLE_TESTS)
    set(UTILS_QT_NEED_GTEST ON)
endif()

if (UTILS_QT_ENABLE_BENCHMARK)
    set(UTILS_QT_NEED_GTEST ON)
    set(UTILS_QT_NEED_BENCHMARK ON)
endif()

if (NOT ${UTILS_QT_GTEST_SEARCH_MODE} STREQUAL "Skip")
    if (${UTILS_QT_GTEST_SEARCH_MODE} STREQUAL "Auto")
        set(UTILS_QT__INTERNAL_GTEST_SEARCH_MODE QUIET)
    else()
        set(UTILS_QT__INTERNAL_GTEST_SEARCH_MODE REQUIRED)
    endif()
endif()

if (UTILS_QT_NEED_GTEST AND NOT TARGET gtest)
    if (NOT ${UTILS_QT_GTEST_SEARCH_MODE} STREQUAL "Skip")
        find_package(GTest ${UTILS_QT__INTERNAL_GTEST_SEARCH_MODE})
    endif()

    if (GTest_FOUND)
        message("GTest found!")
    else()
        message("GTest NOT found! Adding external source...")

        set(INSTALL_GTEST OFF CACHE BOOL "Do not install gtest" FORCE)
        set(INSTALL_GMOCK OFF CACHE BOOL "Do not install gmock" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
        )

        FetchContent_MakeAvailable(googletest)
    endif()
endif()

if (UTILS_QT_NEED_BENCHMARK AND NOT TARGET benchmark)
    if (NOT ${UTILS_QT_GTEST_SEARCH_MODE} STREQUAL "Skip")
        find_package(benchmark ${UTILS_QT__INTERNAL_GTEST_SEARCH_MODE})
    endif()
    
    if (benchmark_FOUND)
        message("Google benchmark found!")
    else()
        message("Google benchmark NOT found! Adding external source...")

        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            google-benchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
        )

        FetchContent_MakeAvailable(google-benchmark)
    endif()
endif()

enable_testing()

if (UTILS_QT_ENABLE_TESTS)
    add_subdirectory(test)
    add_subdirectory(test2)
    add_subdirectory(test3_qml_manual)
    add_subdirectory(test4_qml_auto)
    add_subdirectory(test5_qml)
endif()

if (UTILS_QT_ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif()
