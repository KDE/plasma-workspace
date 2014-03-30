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

add_library(kded_appmenu MODULE ${kded_appmenu_SRCS})

target_link_libraries(kded_appmenu
    Qt5::DBus
    Qt5::X11Extras
    KF5::DBusAddons
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::WindowSystem
    KF5::KDE4Support
    ${X11_LIBRARIES}
    dbusmenu-qt5
)

install(TARGETS kded_appmenu  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES appmenu.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES com.canonical.AppMenu.Registrar.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES org.kde.kappmenu.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
