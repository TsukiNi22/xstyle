set(TARGET unit_tests)

# =========================
# Compilateur & Standard
# =========================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}) # TARGET destination

# =========================
# Requirement
# =========================
if(CMAKE_CROSSCOMPILING)
    find_package(GTest CONFIG QUIET) # built for the target in the sysroot (ex: mingw64-unit-tests image)
endif()
if(CMAKE_CROSSCOMPILING AND NOT GTest_FOUND)
    # No GTest package for the target (ex: MinGW): built from the sources
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
elseif(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
endif()

# =========================
# Sources
# =========================
# Project sources: the library target when it exists, else the shared list of the parent (never its entry point)
set(SRC_PROJECT "")
if(NOT XSTYLE_PACKAGE_LIB)
    set(SRC_PROJECT ${SRC_SHARED})
    list(TRANSFORM SRC_PROJECT PREPEND ${CMAKE_SOURCE_DIR}/)
endif()

set(SRC
    # Rules
    xstyle/Rules.cpp
)
add_executable(${TARGET} ${SRC} ${SRC_PROJECT})

# =========================
# Dependencies
# =========================
target_include_directories(${TARGET} PRIVATE ../include/ ${CMAKE_BINARY_DIR}/generated .)
target_link_libraries(${TARGET} PRIVATE utils::utils GTest::gtest GTest::gtest_main)
if(XSTYLE_PACKAGE_LIB)
    target_link_libraries(${TARGET} PRIVATE ${TARGET_LIB})
endif()
if(WIN32)
    target_link_options(${TARGET} PRIVATE -static-libgcc -static-libstdc++ -static) # standalone .exe (no MinGW runtime dll)
endif()

# =========================
# Modes
# =========================
target_compile_options(${TARGET} PRIVATE -g -ggdb3)
if(NOT WIN32)
    # libutils_asan.a needs the AddressSanitizer runtime at the link
    target_compile_options(${TARGET} PRIVATE $<$<CONFIG:Asan>:-fsanitize=address -fno-omit-frame-pointer>)
    target_link_options(${TARGET} PRIVATE $<$<CONFIG:Asan>:-fsanitize=address>)
endif()

include(GoogleTest)
if(CMAKE_CROSSCOMPILING)
    # Listed when ctest runs (CMAKE_CROSSCOMPILING_EMULATOR, ex: wine), not at the build
    gtest_discover_tests(${TARGET}
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 120 # first start of wine
    )
else()
    gtest_discover_tests(${TARGET})
endif()

# Command line: the issues of a file written by the test (never committed: its issues would block the hook)
if(XSTYLE_PACKAGE_BIN AND NOT CMAKE_CROSSCOMPILING)
    set(CLI_FILE ${CMAKE_BINARY_DIR}/tests/cli/sample.py)
    add_test(NAME Cli.ReportsIssues
        COMMAND sh -c "mkdir -p $(dirname $1) && printf 'x = 1  \\n\\tpass\\n' > $1 && $0 --rtk --r34 $1; exit 0" $<TARGET_FILE:xstyle> ${CLI_FILE}
    )
    set_tests_properties(Cli.ReportsIssues PROPERTIES PASS_REGULAR_EXPRESSION "G-TRAILING.*|G-TAB")
    add_test(NAME Cli.Version COMMAND $<TARGET_FILE:xstyle> --version)
    add_test(NAME Cli.ListRulesColumns COMMAND $<TARGET_FILE:xstyle> --list-rules --no-color)
    set_tests_properties(Cli.ListRulesColumns PROPERTIES PASS_REGULAR_EXPRESSION "cmake  +Indentation that is not a multiple of 4")
    set_tests_properties(Cli.Version PROPERTIES PASS_REGULAR_EXPRESSION "xstyle ${PROJECT_VERSION}")
endif()
