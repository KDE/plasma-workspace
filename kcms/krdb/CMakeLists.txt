add_definitions(-DTRANSLATION_DOMAIN=\"krdb\")

add_library(krdb krdb.cpp)

generate_export_header(krdb)

target_include_directories(krdb PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/krdb>")

target_link_libraries(krdb PRIVATE Qt::Widgets Qt::DBus KF6::CoreAddons KF6::DBusAddons KF6::GuiAddons KF6::I18n KF6::WindowSystem KF6::ColorScheme X11::X11 Qt::GuiPrivate)

if(HAVE_X11)
  target_link_libraries(krdb PRIVATE XCB::XCB XCB::CURSOR)
endif()

install(TARGETS krdb ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

install(TARGETS krdb EXPORT krdbLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES krdb.h
    ${CMAKE_CURRENT_BINARY_DIR}/krdb_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/krdb COMPONENT Devel
)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/Krdb)
configure_package_config_file(KrdbConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/KrdbConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/KrdbConfigVersion.cmake
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/KrdbConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/KrdbConfigVersion.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    EXPORT krdbLibraryTargets
    NAMESPACE PW::
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    FILE krdbLibraryTargets.cmake
)
