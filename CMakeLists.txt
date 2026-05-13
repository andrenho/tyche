cmake_minimum_required(VERSION 3.20)
project(tyche
        VERSION 0.1.0
        LANGUAGES C
)

# ---------------------------------------------------------------------------
# Project-wide settings
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Default to Release if not explicitly set (single-config generators only)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# Toggle between STATIC and SHARED for the main library
option(BUILD_SHARED_LIB "Build mylib as a shared library" OFF)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Warning flags loaded from external files
# ---------------------------------------------------------------------------
# Each file contains one flag per line, e.g.:
#   -Wall
#   -Wextra
#   -Wshadow
# Lines starting with `#` are treated as comments.
function(read_flags_file path out_var)
    set(flags "")
    if(EXISTS "${path}")
        file(STRINGS "${path}" lines)
        foreach(line IN LISTS lines)
            string(STRIP "${line}" line)
            if(line AND NOT line MATCHES "^#")
                list(APPEND flags "${line}")
            endif()
        endforeach()
    else()
        message(WARNING "Warnings file not found: ${path}")
    endif()
    set(${out_var} "${flags}" PARENT_SCOPE)
endfunction()

set(WARNINGS_DIR "${CMAKE_SOURCE_DIR}/cmake/warnings")
read_flags_file("${WARNINGS_DIR}/common.txt" COMMON_WARNINGS)
read_flags_file("${WARNINGS_DIR}/gcc.txt"    GCC_WARNINGS)
read_flags_file("${WARNINGS_DIR}/clang.txt"  CLANG_WARNINGS)

# Build a single list of warnings appropriate for the current compiler
set(PROJECT_WARNINGS ${COMMON_WARNINGS})
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND PROJECT_WARNINGS ${GCC_WARNINGS})
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang") # matches Clang and AppleClang
    list(APPEND PROJECT_WARNINGS ${CLANG_WARNINGS})
endif()

# ---------------------------------------------------------------------------
# Per-config compile/link options exposed via an INTERFACE target
# ---------------------------------------------------------------------------
# Anything that links `project_options` inherits warnings + debug/release tweaks.
add_library(project_options INTERFACE)

# Warnings apply to every build type
target_compile_options(project_options INTERFACE ${PROJECT_WARNINGS})

# Debug-only flags
target_compile_options(project_options INTERFACE
        "$<$<CONFIG:Debug>:-O0>"
        "$<$<CONFIG:Debug>:-g3>"
        "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
        "$<$<CONFIG:Debug>:-fsanitize=address>"
        "$<$<CONFIG:Debug>:-fsanitize=undefined>"
)
target_link_options(project_options INTERFACE
        "$<$<CONFIG:Debug>:-fsanitize=address>"
        "$<$<CONFIG:Debug>:-fsanitize=undefined>"
)

# LeakSanitizer is not supported on macOS — gate it on platform.
# (On Linux it's already bundled into ASan, but adding the flag is harmless
# and makes intent explicit; on macOS it would fail to link.)
if(NOT APPLE)
    target_compile_options(project_options INTERFACE
            "$<$<CONFIG:Debug>:-fsanitize=leak>"
    )
    target_link_options(project_options INTERFACE
            "$<$<CONFIG:Debug>:-fsanitize=leak>"
    )
endif()

target_compile_definitions(project_options INTERFACE
        "$<$<CONFIG:Debug>:DEBUG_BUILD=1>"
)

# Release-only flags
target_compile_options(project_options INTERFACE
        "$<$<CONFIG:Release>:-O3>"
        "$<$<CONFIG:Release>:-DNDEBUG>"
)
target_link_options(project_options INTERFACE
        "$<$<CONFIG:Release>:-Wl,--as-needed>"
)

# ---------------------------------------------------------------------------
# The library
# ---------------------------------------------------------------------------
if(BUILD_SHARED_LIB)
    set(MYLIB_KIND SHARED)
else()
    set(MYLIB_KIND STATIC)
endif()

add_library(mylib ${MYLIB_KIND}
        src/mylib/foo.c
        src/mylib/bar.c
)
add_library(MyProject::mylib ALIAS mylib)

target_include_directories(mylib
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(mylib PRIVATE project_options)

set_target_properties(mylib PROPERTIES
        VERSION   ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        POSITION_INDEPENDENT_CODE ON
)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(myapp src/app/main.c)
target_link_libraries(myapp PRIVATE MyProject::mylib project_options)

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
enable_testing()
add_executable(mylib_tests tests/test_main.c)
target_link_libraries(mylib_tests PRIVATE MyProject::mylib project_options)
add_test(NAME mylib_tests COMMAND mylib_tests)

# ---------------------------------------------------------------------------
# Install rules (executable and library only — tests excluded)
# ---------------------------------------------------------------------------
install(TARGETS mylib myapp
        EXPORT MyProjectTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)

# Optional: export a CMake package config so downstream projects can do
#   find_package(MyProject CONFIG REQUIRED)
install(EXPORT MyProjectTargets
        FILE        MyProjectTargets.cmake
        NAMESPACE   MyProject::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MyProject
)
