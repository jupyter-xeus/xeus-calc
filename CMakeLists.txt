cmake_minimum_required(VERSION 3.4.3)
project(xeus-calc)

set(XEUS_CALC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XEUS_CALC_INCLUDE_DIR}/xeus-calc/xeus_calc_config.hpp" xcalc_version_defines
     REGEX "#define XCALC_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${xcalc_version_defines})
    if(ver MATCHES "#define XCALC_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XCALC_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(${PROJECT_NAME}_VERSION
${XCALC_VERSION_MAJOR}.${XPYT_VERSION_MINOR}.${XPYT_VERSION_PATCH})
message(STATUS "Building xeus-calc v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

# We generate the kernel.json file, given the installation prefix and the executable name
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xcalc/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xcalc/kernel.json"
)

# Dependencies
# ============

# Be sure to use recent versions
set(xeus_REQUIRED_VERSION 0.19.1)
set(cppzmq_REQUIRED_VERSION 4.3.0)

find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
find_package(cppzmq ${cppzmq_REQUIRED_VERSION} REQUIRED)

# Flags
# =====

include(CheckCXXCompilerFlag)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)

    if (HAS_CPP14_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++14 support!")
    endif()
endif()

# Target and link
# ===============

# xeus-calc source directory
set(XEUS_CALC_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# xeus-calc source files
set(XEUS_CALC_SRC
    ${XEUS_CALC_SRC_DIR}/xeus_calc_interpreter.cpp
)

set(XEUS_CALC_HEADERS
    include/xeus-calc/xeus_calc_interpreter.hpp
    include/xeus-calc/xeus_calc_config.hpp
)

# xeus-calc is the target for the library
add_library(xeus-calc SHARED ${XEUS_CALC_SRC} ${XEUS_CALC_HEADERS})

# xcalc is the target for the kernel executable
add_executable(xcalc src/main.cpp)
target_link_libraries(xcalc PRIVATE xeus-calc)

set_target_properties(xcalc xeus-calc PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

OPTION(DOWNLOAD_GTEST "build gtest from downloaded sources" OFF)

#Option for GTest
if(DOWNLOAD_GTEST OR GTEST_SRC_DIR)
    set(BUILD_TESTS ON)
endif()


# take into account the CMakeList in test
if(BUILD_TESTS)
    add_subdirectory(test)
endif()

target_link_libraries(xeus-calc PUBLIC xeus)

if (APPLE)
    set_target_properties(xcalc PROPERTIES
        MACOSX_RPATH ON
    )
else()
    set_target_properties(xcalc PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        SKIP_BUILD_RPATH FALSE
    )
endif()

set_target_properties(xcalc PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

target_include_directories(xeus-calc
                           PUBLIC
                           ${XCALC_INCLUDE_DIRS}
                           $<BUILD_INTERFACE:${XEUS_CALC_INCLUDE_DIR}>
                           $<INSTALL_INTERFACE:include>)

set_target_properties(xeus-calc PROPERTIES
                      PUBLIC_HEADER "${XEUS_CALC_HEADERS}"
                      COMPILE_DEFINITIONS "XEUS_CALC_EXPORTS"
                      PREFIX ""
                      VERSION ${${PROJECT_NAME}_VERSION}
                      SOVERSION ${XCALC_VERSION_MAJOR}
OUTPUT_NAME "libxeus-calc")


# Installation
# ============

# Install xeus-calc and xcalc
install(TARGETS xeus-calc
        EXPORT ${PROJECT_NAME}-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xeus-calc)

install(TARGETS xcalc
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Configuration and data directories for jupyter and xeus_calc
set(XJUPYTER_DATA_DIR "share/jupyter" CACHE STRING "Jupyter data directory")

# Install Jupyter kernelspecs
set(XEUS_CALCSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
install(DIRECTORY ${XEUS_CALCSPEC_DIR}
        DESTINATION ${XJUPYTER_DATA_DIR}
        PATTERN "*.in" EXCLUDE)
