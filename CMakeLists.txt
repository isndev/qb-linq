cmake_minimum_required(VERSION 3.22)

# -----------------------------------------------------------------------------
# qb-linq — header-only query library (qb framework)
# -----------------------------------------------------------------------------
project(qb-linq
    VERSION 1.0.0
    DESCRIPTION "Header-only lazy LINQ-style queries for C++17 (qb framework)"
    LANGUAGES CXX
)

include(GNUInstallDirs)

# Generated public header: single source of truth is project(VERSION …) above.
set(QB_LINQ_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_include")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${QB_LINQ_GENERATED_INCLUDE_DIR}/qb/linq/version.h"
    @ONLY
)

# Top-level vs subdirectory: enable tests by default only when this is the root project.
if(PROJECT_IS_TOP_LEVEL)
    set(_qb_default_build_tests ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
else()
    set(_qb_default_build_tests OFF)
endif()

option(QB_BUILD_TESTS "Build GoogleTest suite (ctest)" ${_qb_default_build_tests})
option(QB_BUILD_BENCHMARKS "Build Google Benchmark executable qb_linq_benchmark" OFF)
# ON: configures doc/Doxyfile.in and adds target qb_linq_docs (HTML under ${CMAKE_BINARY_DIR}/doc_doxygen/html).
# HTML: Doxygen Awesome + extensions (dark mode, copy-on-hover, heading links, interactive TOC);
# custom header doc/doxygen/header.html; README.md is the main page.
# Configure preset "docs" in CMakePresets.json or pass -DQB_LINQ_BUILD_DOCS=ON. Requires doxygen on PATH.
option(QB_LINQ_BUILD_DOCS "Build API reference with Doxygen (HTML)" OFF)

# -----------------------------------------------------------------------------
# Interface library — link as qb::linq (exported INSTALL name)
# -----------------------------------------------------------------------------
add_library(qb-linq INTERFACE)
add_library(qb::linq ALIAS qb-linq)

target_include_directories(qb-linq INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${QB_LINQ_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(qb-linq INTERFACE cxx_std_17)

set_target_properties(qb-linq PROPERTIES
    EXPORT_NAME linq
)

# -----------------------------------------------------------------------------
# Optional: tests, benchmarks, documentation
# -----------------------------------------------------------------------------
if(QB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(QB_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(QB_LINQ_BUILD_DOCS)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        set(QB_LINQ_DOXYFILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in")
        set(QB_LINQ_DOXYFILE_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.qb-linq")
        # QB_LINQ_GENERATED_INCLUDE_DIR is set at top level (version.h for Doxygen INPUT)
        configure_file("${QB_LINQ_DOXYFILE_IN}" "${QB_LINQ_DOXYFILE_OUT}" @ONLY)
        add_custom_target(qb_linq_docs
            COMMAND "${DOXYGEN_EXECUTABLE}" "${QB_LINQ_DOXYFILE_OUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Doxygen HTML -> ${CMAKE_CURRENT_BINARY_DIR}/doc_doxygen/html (warnings: ${CMAKE_CURRENT_BINARY_DIR}/doxygen-qb-linq.warnings.log)"
            VERBATIM
        )
    else()
        message(WARNING "QB_LINQ_BUILD_DOCS is ON but Doxygen was not found; target qb_linq_docs is skipped.")
    endif()
endif()

# -----------------------------------------------------------------------------
# Install + CMake package: find_package(qb-linq CONFIG) → qb::linq
# -----------------------------------------------------------------------------
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES "${QB_LINQ_GENERATED_INCLUDE_DIR}/qb/linq/version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qb/linq")

install(TARGETS qb-linq
    EXPORT qb-linq-targets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT qb-linq-targets
    FILE qb-linq-targets.cmake
    NAMESPACE qb::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/qb-linq"
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qb-linq-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/qb-linq-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/qb-linq"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/qb-linq-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/qb-linq-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/qb-linq-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/qb-linq"
)
