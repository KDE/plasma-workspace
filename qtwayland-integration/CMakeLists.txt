add_library(plasma-shell MODULE)

target_sources(plasma-shell PRIVATE
    main.cpp
    qwaylandxdgdecorationv1.cpp
    qwaylandxdgactivationv1.cpp
    qwaylandxdgshell.cpp
    qwaylandxdgshellintegration.cpp
    qwaylandxdgexporterv2.cpp
)

target_link_libraries(plasma-shell PRIVATE
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::WaylandClient
    Qt::WaylandClientPrivate
    Wayland::Client
    PkgConfig::XKBCOMMON
    LayerShellQt::Interface
)

qt6_generate_wayland_protocol_client_sources(plasma-shell
    FILES
        ${WaylandProtocols_DATADIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
        ${WaylandProtocols_DATADIR}/unstable/xdg-foreign/xdg-foreign-unstable-v2.xml
        ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
        ${WaylandProtocols_DATADIR}/staging/xdg-activation/xdg-activation-v1.xml
)

if (TARGET Qt::XkbCommonSupportPrivate)
    target_link_libraries(plasma-shell Qt::XkbCommonSupportPrivate)
endif()

install(TARGETS plasma-shell
        LIBRARY DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/wayland-shell-integration)
