include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(taskmanagerplugin SHARED taskmanagerplugin.cpp)

target_link_libraries(taskmanagerplugin taskmanager)

install(TARGETS taskmanagerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)

ecm_qt_declare_logging_category(SRCS
                            HEADER logging.h
                            IDENTIFIER PIPEWIRE_LOGGING
                            CATEGORY_NAME plasma_workspace_pipewire_logging
                            DESCRIPTION "PipeWire components for window thumbnails"
                            EXPORT PlasmaWorkspacePipeWire)
ecm_qt_install_logging_categories(
    EXPORT PlasmaWorkspacePipeWire
    FILE myproject.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_client_protocol(SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml
        BASENAME zkde-screencast-unstable-v1
    )
else()
    qt6_generate_wayland_protocol_client_sources(taskmanagerplugin FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml)
endif()
target_include_directories(taskmanagerplugin PRIVATE ${Libdrm_INCLUDE_DIR})

target_sources(taskmanagerplugin PUBLIC screencasting.cpp screencastingrequest.cpp ${SRCS})
target_link_libraries(taskmanagerplugin Qt::Qml Qt::GuiPrivate
                                        KF5::I18n
                                        Qt::WaylandClient
                                        Wayland::Client)
if (QT_MAJOR_VERSION EQUAL "5")
    if (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GLESv2")
        target_link_libraries(taskmanagerplugin Qt5::Gui_GLESv2)
    elseif(TARGET Qt5::Gui_GL)
        target_link_libraries(taskmanagerplugin Qt5::Gui_GL)
    endif()
    if (TARGET Qt5::Gui_EGL)
        target_link_libraries(taskmanagerplugin Qt5::Gui_EGL)
    endif()
endif()

if(TARGET K::KPipeWire)
    target_compile_definitions(taskmanagerplugin PRIVATE -DWITH_KPIPEWIRE)
    target_link_libraries(taskmanagerplugin PkgConfig::PipeWire
                                            K::KPipeWire)
endif()
