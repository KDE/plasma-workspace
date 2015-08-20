include_directories(${dbusmenu-qt5_INCLUDE_DIRS})

remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_KEYWORDS)

set(kded_appmenu_SRCS
    appmenu.cpp
    menuimporter.cpp
    appmenu_dbus.cpp
#     menubutton.cpp
#     menuwidget.cpp
#     menubar.cpp
#     topmenubar.cpp
#     glowbar.cpp
    verticalmenu.cpp
#     shadows.cpp
    )

qt5_add_dbus_adaptor(kded_appmenu_SRCS com.canonical.AppMenu.Registrar.xml
    menuimporter.h MenuImporter menuimporteradaptor MenuImporterAdaptor)

qt5_add_dbus_adaptor(kded_appmenu_SRCS org.kde.kappmenu.xml
    appmenu_dbus.h AppmenuDBus appmenuadaptor AppmenuAdaptor)

add_library(appmenu MODULE ${kded_appmenu_SRCS})
kcoreaddons_desktop_to_json(appmenu appmenu.desktop)

target_link_libraries(appmenu
    Qt5::DBus
    Qt5::X11Extras
    KF5::DBusAddons
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::WindowSystem
    KF5::KDELibs4Support
    ${X11_LIBRARIES}
    dbusmenu-qt5
)

install(TARGETS appmenu DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded )

########### install files ###############

install( FILES com.canonical.AppMenu.Registrar.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
install( FILES org.kde.kappmenu.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
