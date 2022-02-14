include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(taskmanagerplugin SHARED taskmanagerplugin.cpp)

target_link_libraries(taskmanagerplugin taskmanager)

install(TARGETS taskmanagerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)

if(TARGET PkgConfig::PipeWire)
    pkg_check_modules(AVCodec IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUtil IMPORTED_TARGET libavutil)
    pkg_check_modules(AVFormat IMPORTED_TARGET libavformat)
    pkg_check_modules(GBM IMPORTED_TARGET gbm)
    pkg_check_modules(SWScale IMPORTED_TARGET libswscale)
    find_package(epoxy 1.3 REQUIRED)

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

    ecm_add_qtwayland_client_protocol(SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/screencast.xml
        BASENAME zkde-screencast-unstable-v1
    )
    target_include_directories(taskmanagerplugin PRIVATE ${Libdrm_INCLUDE_DIR})

    target_compile_definitions(taskmanagerplugin PRIVATE -DWITH_PIPEWIRE)
    target_sources(taskmanagerplugin PUBLIC pipewirecore.cpp pipewiresourceitem.cpp pipewiresourcestream.cpp pipewirerecord.cpp screencasting.cpp screencastingrequest.cpp ${SRCS})
    target_link_libraries(taskmanagerplugin PkgConfig::PipeWire
                                            PkgConfig::AVCodec PkgConfig::AVUtil PkgConfig::AVFormat PkgConfig::GBM PkgConfig::SWScale epoxy::epoxy # PipeWireRecord
                                            Qt::Qml Qt5::Gui_EGL Qt::GuiPrivate

                                            KF5::I18n
                                            KF5::WaylandClient
                                            Wayland::Client)
    if (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GLESv2")
        target_link_libraries(taskmanagerplugin Qt5::Gui_GLESv2)
    else()
        target_link_libraries(taskmanagerplugin Qt5::Gui_GL)
    endif()
endif()
