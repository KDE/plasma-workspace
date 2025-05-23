find_package(XCB
    REQUIRED COMPONENTS
        XCB
        XFIXES
        DAMAGE
        COMPOSITE
        RANDR
        SHM
        UTIL
        IMAGE
)

set(XCB_LIBS
    XCB::XCB
    XCB::XFIXES
    XCB::DAMAGE
    XCB::COMPOSITE
    XCB::RANDR
    XCB::SHM
    XCB::UTIL
    XCB::IMAGE
)

set(XEMBED_SNI_PROXY_SOURCES
    main.cpp
    fdoselectionmanager.cpp fdoselectionmanager.h
    snidbus.cpp snidbus.h
    sniproxy.cpp
    xtestsender.cpp xtestsender.h
 )

qt_add_dbus_adaptor(XEMBED_SNI_PROXY_SOURCES org.kde.StatusNotifierItem.xml
                     sniproxy.h SNIProxy)

set(statusnotifierwatcher_xml org.kde.StatusNotifierWatcher.xml)
qt_add_dbus_interface(XEMBED_SNI_PROXY_SOURCES ${statusnotifierwatcher_xml} statusnotifierwatcher_interface)

ecm_qt_declare_logging_category(XEMBED_SNI_PROXY_SOURCES HEADER debug.h
                                               IDENTIFIER SNIPROXY
                                               CATEGORY_NAME kde.xembedsniproxy
                                               DEFAULT_SEVERITY Info
                                               DESCRIPTION "xembed sni proxy"
                                               EXPORT PLASMAWORKSPACE
                                           )

add_executable(xembedsniproxy ${XEMBED_SNI_PROXY_SOURCES})



set_package_properties(XCB PROPERTIES TYPE REQUIRED)


target_link_libraries(xembedsniproxy
    Qt::Core
    Qt::DBus
    KF6::DBusAddons
    KF6::WindowSystem
    KF6::CoreAddons
    KF6::Crash
    ${XCB_LIBS}
    X11::Xtst
)

install(TARGETS xembedsniproxy ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES xembedsniproxy.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

ecm_install_configured_files(INPUT plasma-xembedsniproxy.service.in @ONLY DESTINATION  ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
