project(kglobalaccel)

add_definitions(-DTRANSLATION_DOMAIN=\"kglobalaccel\")

###############################################################################
### KDED Global Accel Daemon

set(kglobalaccel_SRCS
    main.cpp
    kglobalacceld.cpp
    component.cpp
    globalshortcut.cpp
    globalshortcutsregistry.cpp
    globalshortcutcontext.cpp)

set(HAVE_X11 0)
if (${XCB_XCB_FOUND} AND ${XCB_KEYSYMS_FOUND})
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_x11.cpp )
  set(HAVE_X11 1)
endif()
if ( Q_WS_MAC )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_mac.cpp )
endif ()
if ( Q_WS_WIN )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_win.cpp )
endif ()
if ( Q_WS_QWS )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_qws.cpp )
endif ()

configure_file(config-kglobalaccel.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kglobalaccel.h )

# kf5_add_kdeinit_executable( kglobalaccel NOGUI ${kglobalaccel_SRCS} )
kf5_add_kdeinit_executable( kglobalaccel5 ${kglobalaccel_SRCS} )
target_link_libraries(kdeinit_kglobalaccel5
   Qt5::DBus
   KF5::GlobalAccel
   KF5::I18n
   KF5::WindowSystem # KKeyServer
   KF5::DBusAddons   # KDBusService
   KF5::CoreAddons   # KAboutData
   KF5::ConfigCore
   KF5::Crash)

if (${XCB_XCB_FOUND} AND ${XCB_KEYSYMS_FOUND})
    target_link_libraries(kdeinit_kglobalaccel5 Qt5::X11Extras XCB::XCB XCB::KEYSYMS)
endif()

if(Q_WS_MAC)
   target_link_libraries(kdeinit_kglobalaccel5 ${CARBON_LIBRARY})
endif()
target_link_libraries(kglobalaccel5 kdeinit_kglobalaccel5)

# Install application and configuration
install( TARGETS kdeinit_kglobalaccel5 ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( TARGETS kglobalaccel5 ${INSTALL_TARGETS_DEFAULT_ARGS} )
# install( FILES kglobalaccel.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kglobalaccel.desktop  DESTINATION ${SERVICES_INSTALL_DIR} RENAME kglobalaccel5.desktop)

configure_file(org.kde.kglobalaccel.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kglobalaccel.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kglobalaccel.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
