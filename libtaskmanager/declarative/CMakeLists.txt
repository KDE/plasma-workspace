include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_client_protocol(SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml
        BASENAME zkde-screencast-unstable-v1
    )
else()
    qt6_generate_wayland_protocol_client_sources(taskmanagerplugin FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml)
endif()

ecm_qt_declare_logging_category(SRCS
                            HEADER logging.h
                            IDENTIFIER TMQML_LOGGING
                            CATEGORY_NAME plasma_workspace_taskmanager_declarative_logging
                            DESCRIPTION "QML components for window thumbnails"
                            EXPORT PlasmaWorkspaceTaskManagerDeclarative)
ecm_qt_install_logging_categories(
    EXPORT PlasmaWorkspaceTaskManagerDeclarative
    FILE PlasmaWorkspaceTaskManagerDeclarative.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

add_library(taskmanagerplugin SHARED taskmanagerplugin.cpp screencasting.cpp screencastingrequest.cpp ${SRCS})
target_link_libraries(taskmanagerplugin taskmanager Wayland::Client KF5::WaylandClient)

install(TARGETS taskmanagerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)
