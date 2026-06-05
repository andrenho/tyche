cmake_minimum_required (VERSION 3.24...4.3.3)

project(tyche VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find things

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(IS_CLANG TRUE)
else()
    set(IS_CLANG FALSE)
endif()


# C/C++ Flags

add_library(tyche_flags INTERFACE)

set(CMAKE_C_STANDARD 99 CACHE STRING "C Standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ Standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(IS_CLANG)
    set(WARNINGS "@${CMAKE_SOURCE_DIR}/config/WARNINGS_CLANG")
else()
    set(WARNINGS "@${CMAKE_SOURCE_DIR}/config/WARNINGS_GCC")
endif()
set(WARNINGS "${WARNINGS}" "@${CMAKE_SOURCE_DIR}/config/WARNINGS")

target_compile_definitions(tyche_flags INTERFACE
        _GNU_SOURCE
        VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        VERSION_MINOR=${PROJECT_VERSION_MINOR}
)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/lib/contrib)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/contrib)
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Debugging flags

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZE_FLAGS -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        list(APPEND SANITIZE_FLAGS -fsanitize=leak)
    endif()

    target_compile_options(tyche_flags INTERFACE
            -O0 -ggdb3 ${WARNINGS}
            -fno-omit-frame-pointer
            -fstack-protector-strong -fstack-clash-protection -fno-common
            ${SANITIZE_FLAGS} ${ADD_DBG_FLAGS}
    )

    if (NOT CLANG)
        target_compile_options(tyche_flags INTERFACE -fanalyzer)
    endif()
    add_compile_definitions(CHECK_TYCHE_BUGS=1)
    target_link_options(tyche_flags INTERFACE ${SANITIZE_FLAGS})

else()  # RELEASE

    target_compile_options(tyche_flags INTERFACE -O3 -flto=auto -march=native -mtune=native
            -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -fstack-protector-strong)
    target_link_options(tyche_flags INTERFACE -flto=auto -s)

endif()

#
# code generation: instructions.h / instructions.c
#

add_custom_command(
        OUTPUT
        ${CMAKE_SOURCE_DIR}/lib/instructions/instructions.h
        ${CMAKE_SOURCE_DIR}/lib/instructions/instructions.c
        COMMAND ./gen-inst.lua
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/instructions
        DEPENDS ${CMAKE_SOURCE_DIR}/lib/instructions/gen-inst.lua
        COMMENT "Generating instructions"
)
add_custom_target(gen_instructions
        DEPENDS ${CMAKE_SOURCE_DIR}/lib/instructions/instructions.h
)

#
# library
#

set(LIB_SRC
        lib/assembler/assembler_priv.h
        lib/assembler/assembly.c
        lib/assembler/assembler.c
        lib/assembler/adj_labels.c
        lib/assembler/bytecode.c
        lib/instructions/instructions.h
        lib/instructions/instructions.c
        lib/priv.h
        lib/tyche.h
        lib/utils.c
        lib/code.c
        lib/value.c
        lib/stack.c
        #lib/array.c
        #lib/table.c
        #lib/heap.c
        #lib/vm.c
        #lib/expr.c
)

# static library
add_library(tyche_static STATIC ${LIB_SRC})
set_target_properties(tyche_static PROPERTIES OUTPUT_NAME tyche)
target_link_libraries(tyche_static PRIVATE tyche_flags)
add_dependencies(tyche_static gen_instructions)

# shared library
add_library(tyche_shared SHARED ${LIB_SRC})
set_target_properties(tyche_shared PROPERTIES
        OUTPUT_NAME tyche
        VERSION ${PROJECT_VERSION}
)
target_link_libraries(tyche_shared PRIVATE tyche_flags)
add_dependencies(tyche_shared gen_instructions)

enable_testing()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

##
## executable
##
#
#add_executable(tyche src/tyche.c)
#target_link_libraries(tyche PRIVATE tyche_static m)
#add_dependencies(tyche gen_instructions)


#
# tests
#

#find_package(GTest QUIET)
#if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
#endif()

add_executable(tyche-test
        test/tests-assembly.cc
        test/tests-values.cc
        test/tests-stack.cc
)
target_link_libraries(tyche-test PRIVATE tyche_static GTest::gtest_main m)
add_dependencies(tyche-test gen_instructions)
target_link_libraries(tyche-test PRIVATE tyche_flags)

enable_testing()
add_test(NAME tyche-test COMMAND tyche-test)

##
## install
##
#
#include(GNUInstallDirs)
#install(TARGETS tyche tyche_static tyche_shared
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)
#install(FILES lib/tyche.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#
#
##
## packaging
##
#
#set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
#set(CPACK_GENERATOR "TBZ2")
#set(CPACK_SOURCE_GENERATOR "TBZ2")
#set(CPACK_SOURCE_IGNORE_FILES "/build/;/.git/")
#include(CPack)