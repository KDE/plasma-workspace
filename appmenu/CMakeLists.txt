remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_KEYWORDS)

set(kded_appmenu_SRCS
    appmenu.cpp
    menuimporter.cpp
    appmenu_dbus.cpp
    verticalmenu.cpp
    )

qt_add_dbus_adaptor(kded_appmenu_SRCS com.canonical.AppMenu.Registrar.xml
    menuimporter.h MenuImporter menuimporteradaptor MenuImporterAdaptor)

qt_add_dbus_adaptor(kded_appmenu_SRCS org.kde.kappmenu.xml
    appmenu_dbus.h AppmenuDBus appmenuadaptor AppmenuAdaptor)

kcoreaddons_add_plugin(appmenu SOURCES ${kded_appmenu_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kded")

pkg_check_modules(XKBCommon REQUIRED IMPORTED_TARGET xkbcommon)

target_link_libraries(appmenu
    Qt::DBus
    Qt::WaylandClientPrivate
    KF5::DBusAddons
    KF5::KIOCore
    KF5::WaylandClient
    KF5::WindowSystem
    Wayland::Client
    PkgConfig::XKBCommon
    dbusmenuqt
)
if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(appmenu Qt::XkbCommonSupportPrivate)
endif()

if (HAVE_X11)
    target_link_libraries(appmenu XCB::XCB)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(appmenu Qt::X11Extras)
    else()
        target_link_libraries(appmenu Qt::GuiPrivate)
    endif()
endif()

ecm_qt_declare_logging_category(appmenu
    HEADER appmenu_debug.h
    IDENTIFIER APPMENU_DEBUG
    CATEGORY_NAME org.kde.plasma.appmenu)

########### install files ###############

install( FILES com.canonical.AppMenu.Registrar.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
install( FILES org.kde.kappmenu.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
