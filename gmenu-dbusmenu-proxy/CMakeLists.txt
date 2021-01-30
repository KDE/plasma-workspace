find_package(AppMenuGtkModule)
set_package_properties(AppMenuGtkModule PROPERTIES TYPE RUNTIME)

add_definitions(-DQT_NO_CAST_TO_ASCII
-DQT_NO_CAST_FROM_ASCII
-DQT_NO_CAST_FROM_BYTEARRAY)

find_package(XCB
    REQUIRED COMPONENTS
        XCB
)

set(GMENU_DBUSMENU_PROXY_SRCS
    main.cpp
    menuproxy.cpp
    window.cpp
    menu.cpp
    actions.cpp
    gdbusmenutypes_p.cpp
    icons.cpp
    utils.cpp
    ../libdbusmenuqt/dbusmenutypes_p.cpp
 )

qt5_add_dbus_adaptor(GMENU_DBUSMENU_PROXY_SRCS ../libdbusmenuqt/com.canonical.dbusmenu.xml window.h Window)

ecm_qt_declare_logging_category(GMENU_DBUSMENU_PROXY_SRCS HEADER debug.h
                                               IDENTIFIER DBUSMENUPROXY
                                               CATEGORY_NAME kde.dbusmenuproxy
                                               DEFAULT_SEVERITY Info)

add_executable(gmenudbusmenuproxy ${GMENU_DBUSMENU_PROXY_SRCS})

set_package_properties(XCB PROPERTIES TYPE REQUIRED)

target_link_libraries(gmenudbusmenuproxy
    Qt::Core
    Qt::X11Extras
    Qt::DBus
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::WindowSystem
    XCB::XCB
)

install(TARGETS gmenudbusmenuproxy ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES gmenudbusmenuproxy.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

ecm_install_configured_files(INPUT plasma-gmenudbusmenuproxy.service.in @ONLY DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})
