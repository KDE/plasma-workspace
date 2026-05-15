include(CheckCXXSourceCompiles)

set(_appmenu_saved_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
list(APPEND CMAKE_REQUIRED_LIBRARIES Qt::WaylandClientPrivate Qt::GuiPrivate)
check_cxx_source_compiles("
    #include <QtWaylandClient/private/qwaylanddisplay_p.h>
    #include <QtWaylandClient/private/qwaylandwindow_p.h>

    int main()
    {
        auto display = static_cast<QtWaylandClient::QWaylandDisplay *>(nullptr);
        auto device = static_cast<QtWaylandClient::QWaylandInputDevice *>(nullptr);
        auto window = static_cast<QtWaylandClient::QWaylandWindow *>(nullptr);
        display->setLastInputDevice(device, 1, window->waylandSurface());
        return 0;
    }
" HAVE_QT_WAYLAND_SETLASTINPUTDEVICE_SURFACE)
set(CMAKE_REQUIRED_LIBRARIES "${_appmenu_saved_required_libraries}")

set(kded_appmenu_SRCS
    appmenu.cpp
    menuimporter.cpp
    appmenu_dbus.cpp
    verticalmenu.cpp

    appmenu.h
    menuimporter.h
    appmenu_dbus.h
    verticalmenu.h
    )

qt_add_dbus_adaptor(kded_appmenu_SRCS com.canonical.AppMenu.Registrar.xml
    menuimporter.h MenuImporter menuimporteradaptor MenuImporterAdaptor)

qt_add_dbus_adaptor(kded_appmenu_SRCS org.kde.kappmenu.xml
    appmenu_dbus.h AppmenuDBus appmenuadaptor AppmenuAdaptor)

kcoreaddons_add_plugin(appmenu SOURCES ${kded_appmenu_SRCS} INSTALL_NAMESPACE "kf6/kded")

if (HAVE_QT_WAYLAND_SETLASTINPUTDEVICE_SURFACE)
    target_compile_definitions(appmenu PRIVATE HAVE_QT_WAYLAND_SETLASTINPUTDEVICE_SURFACE)
endif()

pkg_check_modules(XKBCommon REQUIRED IMPORTED_TARGET xkbcommon)

target_link_libraries(appmenu
    Qt::DBus
    Qt::WaylandClientPrivate
    KF6::DBusAddons
    KF6::KIOCore
    Plasma::KWaylandClient
    KF6::WindowSystem
    Wayland::Client
    PkgConfig::XKBCommon
    dbusmenuqt
)
if (HAVE_X11)
    target_link_libraries(appmenu XCB::XCB)
endif()

ecm_qt_declare_logging_category(appmenu
    HEADER appmenu_debug.h
    IDENTIFIER APPMENU_DEBUG
    CATEGORY_NAME org.kde.plasma.appmenu
    DESCRIPTION "appmenu"
    EXPORT PLASMAWORKSPACE
)

########### install files ###############

install( FILES com.canonical.AppMenu.Registrar.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
install( FILES org.kde.kappmenu.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
