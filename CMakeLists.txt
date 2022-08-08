# Need at least 3.6 for selection of the VS startup project.
# No restrictions for Linux, but 2.8 is preferred.
if(MSVC)
    cmake_minimum_required(VERSION 3.6)
else()
    cmake_minimum_required(VERSION 2.8.5)
endif()


# Defining the root project (solution)
project(swm-sched CXX)
set(TESTS_DIR              ${PROJECT_SOURCE_DIR}/tests/unit)
set(WM_DIR                 ${PROJECT_SOURCE_DIR}/deps/swm-core/c_src/lib)
set(SCHED_LIB_DIR          ${PROJECT_SOURCE_DIR}/src/sched-lib)
set(SCHED_EXE_DIR          ${PROJECT_SOURCE_DIR}/src/sched-exe)
set(SCHED_TST_DIR          ${TESTS_DIR})
set(PLG_LIB_DIR            ${PROJECT_SOURCE_DIR}/src/plugin-lib)
set(PLG_DUMMY_DIR          ${PROJECT_SOURCE_DIR}/src/plugins/dummy)
set(PLG_FCFS_DIR           ${PROJECT_SOURCE_DIR}/src/plugins/fcfs)
set(PLG_FAIR_SHARING_DIR   ${PROJECT_SOURCE_DIR}/src/plugins/fair-sharing)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
    set(CMAKE_SUPPRESS_REGENERATION ON)
endif()


# Some checks for environment variables and Win64
if (MSVC)
    if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
        message(FATAL_ERROR "Generator Win32 is not supported due to dependency on Erlang 64-bit library")
    endif()
endif()

if(NOT EXISTS $ENV{_KERL_ACTIVE_DIR}/usr/include)
    message("Erlang SDK not found, please set up environment variable _KERL_ACTIVE_DIR")
    message(FATAL_ERROR "Configuration aborted")
endif()

if(EXISTS $ENV{GTEST_ROOT}/include)
    enable_testing()
    find_package(GTest REQUIRED)
else()
    set(GTEST_FOUND OFF)
    message("Google Test SDK not found, please set up environment variable GTEST_ROOT")
    message(WARNING "Unit tests disabled")
endif()


# pThreads, Linux-only
if(NOT MSVC)
    find_package(Threads REQUIRED)
endif()


# External library ei.lib / libei.a
add_library(ei STATIC IMPORTED)
if(MSVC)
    set_target_properties(ei PROPERTIES IMPORTED_LOCATION_DEBUG
                          $ENV{_KERL_ACTIVE_DIR}/usr/lib/ei_mdd.lib)
    set_target_properties(ei PROPERTIES IMPORTED_LOCATION_RELEASE
                          $ENV{_KERL_ACTIVE_DIR}/usr/lib/ei_md.lib)
else()
    set_target_properties(ei PROPERTIES IMPORTED_LOCATION
                          $ENV{_KERL_ACTIVE_DIR}/usr/lib/libei.a)
endif()


# Another external library, Windows-only
if(MSVC)
    add_library(Ws2 STATIC IMPORTED)
    set_target_properties(Ws2 PROPERTIES IMPORTED_LOCATION Ws2_32.lib)
endif()


# Implementation of sscanf(), sprintf(), etc for VS 2015+ (VC++ 12.0+ toolsets).
if(MSVC AND MSVC_VERSION GREATER 1700)
    add_library(vs_legacy STATIC IMPORTED)
    set_target_properties(vs_legacy PROPERTIES IMPORTED_LOCATION legacy_stdio_definitions.lib)
endif()


# Libraries for GTest. Defining them directly because we need unit tests
# for Debug and Release modes. Default linking supports only one mode.
# Not an issue for Linux.
if (MSVC AND GTEST_FOUND)
    add_library(gtest STATIC IMPORTED)
    set_target_properties(gtest PROPERTIES IMPORTED_LOCATION_DEBUG $ENV{GTEST_ROOT}/lib/gtestd.lib)
    set_target_properties(gtest PROPERTIES IMPORTED_LOCATION_RELEASE $ENV{GTEST_ROOT}/lib/gtest.lib)
    add_library(gtest_main STATIC IMPORTED)
    set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION_DEBUG $ENV{GTEST_ROOT}/lib/gtest_maind.lib)
    set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION_RELEASE $ENV{GTEST_ROOT}/lib/gtest_main.lib)
endif()


# Some macros to minimaze copy-paste.

macro(swm_set_gcc_flags target_name)
    if(NOT MSVC)
        set_target_properties(${target_name} PROPERTIES COMPILE_FLAGS
                              "--std=c++17 -Werror -Wno-deprecated-declarations -Wall -Wextra -Wconversion -pedantic")
    endif()
endmacro(swm_set_gcc_flags target_name)

macro(swm_discover_sources src_dir h_files cpp_files)
    file(GLOB_RECURSE ${h_files}   ${src_dir}/*.h)
    file(GLOB_RECURSE ${cpp_files} ${src_dir}/*.cpp)

    if(MSVC)
        foreach(_source IN ITEMS ${${h_files}})
            get_filename_component(_source_path "${_source}" PATH)
            file(RELATIVE_PATH _source_path_rel "${src_dir}" "${_source_path}")
            string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
            source_group("Header Files\\${_group_path}" FILES "${_source}")
        endforeach()

        foreach(_source IN ITEMS ${${cpp_files}})
            get_filename_component(_source_path "${_source}" PATH)
            file(RELATIVE_PATH _source_path_rel "${src_dir}" "${_source_path}")
            string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
            source_group("Source Files\\${_group_path}" FILES "${_source}")
        endforeach()
    endif()
endmacro(swm_discover_sources src_dir h_files cpp_files)

macro(swm_configure_lib target_name out_name)
    target_include_directories(${target_name} SYSTEM PUBLIC $ENV{_KERL_ACTIVE_DIR}/usr/include)
    if(NOT MSVC)
      set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
      swm_set_gcc_flags(${target_name})
    endif()
 
    set_target_properties(${target_name} PROPERTIES PREFIX "")
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${out_name})
if(MSVC)
    set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG
                          ${PROJECT_SOURCE_DIR}/bin)
    set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE
                          ${PROJECT_SOURCE_DIR}/bin)
else()
    set_target_properties(${target_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                          ${PROJECT_SOURCE_DIR}/bin)
endif()
endmacro(swm_configure_lib target_name out_name)

macro(swm_configure_assembly target_name out_name)
    target_include_directories(${target_name} SYSTEM PUBLIC $ENV{_KERL_ACTIVE_DIR}/usr/include)
    target_include_directories(${target_name} PUBLIC ${WM_DIR})
    target_include_directories(${target_name} PUBLIC ${SCHED_LIB_DIR})

    if(MSVC)
      target_link_libraries(${target_name} Ws2)
      if(MSVC_VERSION GREATER 1700)
          target_link_libraries(${target_name} vs_legacy)
      endif()
    else()
      swm_set_gcc_flags(${target_name})
    endif()

    set_target_properties(${target_name} PROPERTIES PREFIX "")
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${out_name})

if(MSVC)
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG
                          ${PROJECT_SOURCE_DIR}/bin)
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE
                          ${PROJECT_SOURCE_DIR}/bin)
    set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG
                          ${PROJECT_SOURCE_DIR}/bin)
    set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE
                          ${PROJECT_SOURCE_DIR}/bin)
else()
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                          ${PROJECT_SOURCE_DIR}/bin)
    set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                          ${PROJECT_SOURCE_DIR}/bin)
endif()
endmacro(swm_configure_assembly target_name out_name)


macro(swm_configure_exe target_name out_name)
    swm_configure_assembly(${target_name} ${out_name})

    # Beware - it's a magic lib order!
    target_link_libraries(${target_name} ei)
    target_link_libraries(${target_name} sched-lib)
    target_link_libraries(${target_name} wm-lib ei)

    if(NOT MSVC)
        target_link_libraries(${target_name} ${CMAKE_DL_LIBS})
        target_link_libraries(${target_name} ${CMAKE_THREAD_LIBS_INIT})
    endif()

endmacro(swm_configure_exe target_name out_name)


macro(swm_configure_test target_name out_name test_headers)
    swm_configure_assembly(${target_name} ${out_name})
    target_include_directories(${target_name} SYSTEM PUBLIC
                               ${GTEST_INCLUDE_DIRS})
    # Beware - it's a magic lib order!
    if(MSVC)
        target_link_libraries(${target_name} gtest gtest_main)
    else()
        target_link_libraries(${target_name} ${GTEST_BOTH_LIBRARIES})
    endif()
    target_link_libraries(${target_name} sched-lib)
    target_link_libraries(${target_name} plugin-lib)
    target_link_libraries(${target_name} wm-lib)
    target_link_libraries(${target_name} ei)

    if(NOT MSVC)
        target_link_libraries(${target_name} ${CMAKE_DL_LIBS})
        target_link_libraries(${target_name} ${CMAKE_THREAD_LIBS_INIT})
    endif()

    if(MSVC)
        set_target_properties(${target_name} PROPERTIES FOLDER tests)
        add_test(NAME ${target_name}  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
                                      COMMAND ${PROJECT_SOURCE_DIR}/bin/${out_name}.exe)
        set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                              "${PROJECT_SOURCE_DIR}/bin")
    else()
        gtest_discover_tests(${target_name} "${PROJECT_SOURCE_DIR}/bin" "${test_headers}")
    endif()
endmacro(swm_configure_test target_name out_name test_headers)

macro(swm_configure_plugin target_name out_name)
swm_configure_assembly(${target_name} ${out_name})
target_include_directories(${target_name} PRIVATE ${PLG_LIB_DIR})
target_link_libraries(${target_name} ei)
target_link_libraries(${target_name} wm-lib)
target_link_libraries(${target_name} plugin-lib)
add_dependencies(${target_name} plugin-lib)
set_target_properties(${target_name} PROPERTIES PREFIX "")
if(MSVC)
    set_target_properties(${target_name} PROPERTIES FOLDER plugins)
endif()
add_dependencies(${target_name} plugin-lib)
endmacro(swm_configure_plugin target_name out_name)


# Library with auto-generated structures for Erlang interop
swm_discover_sources(${WM_DIR} WM_SRC_H WM_SRC_CPP)
add_library(wm-lib STATIC ${WM_SRC_H} ${WM_SRC_CPP})
swm_configure_lib(wm-lib wm)


# Library with all scheduling routines
swm_discover_sources(${SCHED_LIB_DIR} SCHED_LIB_SRC_H SCHED_LIB_SRC_CPP)
add_library(sched-lib STATIC ${SCHED_LIB_SRC_H} ${SCHED_LIB_SRC_CPP})
swm_configure_lib(sched-lib sched-lib)
target_include_directories(sched-lib PRIVATE ${WM_DIR})
target_include_directories(sched-lib PRIVATE ${SCHED_LIB_DIR})
add_dependencies(sched-lib wm-lib)

# Library with basic routines for plugins
swm_discover_sources(${PLG_LIB_DIR} PLG_LIB_SRC_H PLG_LIB_SRC_CPP)
add_library(plugin-lib STATIC ${PLG_LIB_SRC_H} ${PLG_LIB_SRC_CPP})
swm_configure_lib(plugin-lib plugin-lib)
target_include_directories(plugin-lib PRIVATE ${WM_DIR})
target_include_directories(plugin-lib PRIVATE ${SCHED_LIB_DIR})
target_include_directories(plugin-lib PRIVATE ${PLG_LIB_DIR})
add_dependencies(plugin-lib wm-lib)

# Dummy algorithm that is being used in unit tests
swm_discover_sources(${PLG_DUMMY_DIR} PLG_DUMMY_SRC_H PLG_DUMMY_SRC_CPP)
add_library(dummy MODULE ${PLG_DUMMY_SRC_H} ${PLG_DUMMY_SRC_CPP})
swm_configure_plugin(dummy swm-sched-dummy)

# Implementation of FCFS algorithm
swm_discover_sources(${PLG_FCFS_DIR} PLG_FCFS_SRC_H PLG_FCFS_SRC_CPP)
add_library(fcfs MODULE ${PLG_FCFS_SRC_H} ${PLG_FCFS_SRC_CPP})
swm_configure_plugin(fcfs swm-sched-fcfs)

# Implementation of FairSharing algorithm
swm_discover_sources(${PLG_FAIR_SHARING_DIR} PLG_FAIR_SHARING_SRC_H PLG_FAIR_SHARING_SRC_CPP)
add_library(fair-sharing MODULE ${PLG_FAIR_SHARING_SRC_H} ${PLG_FAIR_SHARING_SRC_CPP})
swm_configure_plugin(fair-sharing swm-sched-fair-sharing)

# The main executable target - sched-exe
swm_discover_sources(${SCHED_EXE_DIR} SCHED_EXE_SRC_H SCHED_EXE_SRC_CPP)
add_executable(sched-exe ${SCHED_EXE_SRC_CPP} ${SCHED_EXE_SRC_H})
swm_configure_exe(sched-exe swm-sched)
target_link_libraries(sched-exe sched-lib)
add_dependencies(sched-exe sched-lib)
add_dependencies(sched-exe fcfs)
add_dependencies(sched-exe fair-sharing)
if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT sched-exe)
endif()


# Unit tests for sched-lib and plugin-lib
if(${GTEST_FOUND})
    swm_discover_sources(${SCHED_TST_DIR} SCHED_TST_SRC_H SCHED_TST_SRC_CPP)
    list(APPEND SCHED_TST_SRC_CPP "${SCHED_EXE_DIR}/cli_args.cpp")
    add_executable(sched-tests ${SCHED_TST_SRC_H} ${SCHED_TST_SRC_CPP})
    swm_configure_test(sched-tests swm-sched-tests ${SCHED_TST_SRC_H})
    target_include_directories(sched-tests PRIVATE ${SCHED_TST_DIR})
    target_include_directories(sched-tests PRIVATE ${SCHED_EXE_DIR})
    target_include_directories(sched-tests PRIVATE ${PLG_LIB_DIR})
    add_dependencies(sched-tests sched-exe)
    add_dependencies(sched-tests plugin-lib)
endif()
