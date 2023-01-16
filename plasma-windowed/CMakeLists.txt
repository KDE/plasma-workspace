add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

# For plasmoid tests, unstable library
set(plasmawindowed-lib_SRCS
    plasmawindowedcorona.cpp
    plasmawindowedview.cpp
)
add_library(plasmawindowed_plugin SHARED ${plasmawindowed-lib_SRCS})
add_library(PW::PlasmaWindowed ALIAS plasmawindowed_plugin)
set_target_properties(plasmawindowed_plugin PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    EXPORT_NAME PlasmaWindowed
)
target_link_libraries(plasmawindowed_plugin
    PUBLIC
        Qt::Core
        Qt::Quick
        KF5::Plasma
    PRIVATE
        Qt::Qml
        KF5::I18n
        KF5::IconThemes
        KF5::XmlGui
        KF5::PlasmaQuick
        KF5::Notifications
        KF5::Package
)
target_include_directories(plasmawindowed_plugin
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}>"
)
ecm_generate_export_header(plasmawindowed_plugin
    EXPORT_FILE_NAME plasmawindowed_export.h
    BASE_NAME PlasmaWindowed
    GROUP_BASE_NAME PW
    VERSION ${PROJECT_VERSION}
)

set(plasmawindowed-app_SRCS
    main.cpp
)

add_executable(plasmawindowed ${plasmawindowed-app_SRCS})

target_link_libraries(
    plasmawindowed
    PW::PlasmaWindowed
    Qt::Widgets
    KF5::DBusAddons
    KF5::I18n
)

install(TARGETS plasmawindowed ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#even if hidden, the desktop file is needed anyways for kdbusservice::unique
configure_file(org.kde.plasmawindowed.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmawindowed.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmawindowed.desktop DESTINATION ${KDE_INSTALL_APPDIR})

install(TARGETS plasmawindowed_plugin EXPORT plasmawindowedTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES
        plasmawindowedcorona.h
        plasmawindowedview.h
        ${CMAKE_CURRENT_BINARY_DIR}/plasmawindowed_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/plasma/plasmawindowed
    COMPONENT Devel
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/PlasmaWindowedConfigVersion.cmake
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY ExactVersion
)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/PlasmaWindowed)

configure_package_config_file(PlasmaWindowedConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/PlasmaWindowedConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PlasmaWindowedConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/PlasmaWindowedConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT plasmawindowedTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE PlasmaWindowedLibraryTargets.cmake )