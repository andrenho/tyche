cmake_minimum_required (VERSION 3.24)

project(tyche
        VERSION 0.0.1
        DESCRIPTION "An embeddable/standalone programming language"
        LANGUAGES C CXX ASM)

#
# project options / configuration
#

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ Standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
set_property(GLOBAL PROPERTY LINK_WHAT_YOU_USE TRUE)

# warnings / flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(warnings -Wall -Wextra -Wformat-nonliteral -Wundef -Wshadow -Wwrite-strings -Wfloat-equal -Wswitch-default -Wmissing-format-attribute -Wswitch-enum -Wmissing-noreturn -Wno-unused-parameter -Wno-unused)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(warnings ${warnings} -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=malloc -Wsuggest-attribute=format -Wsuggest-attribute=cold)
    endif()
endif()

# try to use ccache, if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# ignore warnings in imported files
set_source_files_properties(${IMGUI_SRC} PROPERTIES COMPILE_FLAGS "-w")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-ggdb -O0)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(DEF B_PRODUCTION_MODE=ON)
    add_compile_options(-Ofast -flto)
endif()

#
# libraries
#

include(FetchContent)
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

#
# library
#

add_library(lib${PROJECT_NAME} STATIC
        src/bytecode/bytearray.hh
        src/bytecode/bytearray.cc
        src/bytecode/bytecode.cc
        src/bytecode/bytecode.hh
        src/bytecode/bytecodeprototype.hh
        src/common/overloaded.hh
)

target_compile_options(lib${PROJECT_NAME} PRIVATE ${warnings})

#
# tests
#

add_executable(${PROJECT_NAME}-bytecode-test src/bytecode/tests.cc)
target_link_libraries(${PROJECT_NAME}-bytecode-test lib${PROJECT_NAME} gtest_main)
add_test(NAME tyche_bytecode_test COMMAND ${PROJECT_NAME}-bytecode-test)

#
# check for leaks
#

add_custom_target(leaks)
add_custom_command(TARGET leaks
        POST_BUILD
        COMMENT "Check for leaks using valgrind."
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --suppressions=${CMAKE_SOURCE_DIR}/valgrind.supp ./${PROJECT_NAME}
)

#
# installation
#

install(TARGETS lib${CMAKE_PROJECT_NAME} RUNTIME DESTINATION lib)