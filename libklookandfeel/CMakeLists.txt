# SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_library(klookandfeel)
add_library(PW::KLookAndFeel ALIAS klookandfeel)

set_target_properties(klookandfeel PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        EXPORT_NAME KLookAndFeel
        OUTPUT_NAME klookandfeel
)

target_sources(klookandfeel PRIVATE
    klookandfeel.cpp
)

target_link_libraries(klookandfeel
    PUBLIC
        Qt::Core
        Qt::Gui
    PRIVATE
        KF6::ConfigCore
        KF6::Package
)

target_include_directories(klookandfeel
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/klookandfeel>"
)

generate_export_header(klookandfeel)

install(TARGETS klookandfeel EXPORT libklookandfeelLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES
        klookandfeel.h
        ${CMAKE_CURRENT_BINARY_DIR}/klookandfeel_export.h
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/klookandfeel
    COMPONENT Devel
)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/LibKLookAndFeel)
configure_package_config_file(LibKLookAndFeelConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/LibKLookAndFeelConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibKLookAndFeelConfigVersion.cmake
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LibKLookAndFeelConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LibKLookAndFeelConfigVersion.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    EXPORT libklookandfeelLibraryTargets
    NAMESPACE PW::
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    FILE LibKLookAndFeelLibraryTargets.cmake
)
