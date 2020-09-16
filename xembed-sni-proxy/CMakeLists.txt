add_definitions(-DQT_NO_CAST_TO_ASCII
-DQT_NO_CAST_FROM_ASCII
-DQT_NO_URL_CAST_FROM_STRING
-DQT_NO_CAST_FROM_BYTEARRAY)

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
    fdoselectionmanager.cpp
    snidbus.cpp
    sniproxy.cpp
    xtestsender.cpp
 )

qt5_add_dbus_adaptor(XEMBED_SNI_PROXY_SOURCES org.kde.StatusNotifierItem.xml
                     sniproxy.h SNIProxy)

set(statusnotifierwatcher_xml org.kde.StatusNotifierWatcher.xml)
qt5_add_dbus_interface(XEMBED_SNI_PROXY_SOURCES ${statusnotifierwatcher_xml} statusnotifierwatcher_interface)

ecm_qt_declare_logging_category(XEMBED_SNI_PROXY_SOURCES HEADER debug.h
                                               IDENTIFIER SNIPROXY
                                               CATEGORY_NAME kde.xembedsniproxy
                                               DEFAULT_SEVERITY Info)

add_executable(xembedsniproxy ${XEMBED_SNI_PROXY_SOURCES})



set_package_properties(XCB PROPERTIES TYPE REQUIRED)


target_link_libraries(xembedsniproxy
    Qt5::Core
    Qt5::X11Extras
    Qt5::DBus
    KF5::WindowSystem
    ${XCB_LIBS}
    ${X11_XTest_LIB}
)

install(TARGETS xembedsniproxy ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES xembedsniproxy.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

ecm_install_configured_files(INPUT plasma-xembedsniproxy.service.in @ONLY DESTINATION  ${SYSTEMD_USER_UNIT_INSTALL_DIR})
