project(kglobalaccel)

###############################################################################
### KDED Global Accel Daemon

set(kglobalaccel_SRCS
    main.cpp
    kglobalacceld.cpp
    component.cpp
    globalshortcut.cpp
    globalshortcutsregistry.cpp
    globalshortcutcontext.cpp)

if (${XCB_XCB_FOUND} AND ${XCB_KEYSYMS_FOUND})
  include_directories(${XCB_INCLUDE_DIRS})
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_x11.cpp )
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

# kf5_add_kdeinit_executable( kglobalaccel NOGUI ${kglobalaccel_SRCS} )
kf5_add_kdeinit_executable( kglobalaccel5 ${kglobalaccel_SRCS} )
target_link_libraries(kdeinit_kglobalaccel5 Qt5::DBus KF5::GlobalAccel KF5::KCMUtils KF5::I18n KF5::XmlGui KF5::WindowSystem KF5::DBusAddons KF5::Notifications KF5::KIOWidgets KF5::Crash)

if (${XCB_XCB_FOUND} AND ${XCB_KEYSYMS_FOUND})
    target_link_libraries(kdeinit_kglobalaccel5 Qt5::X11Extras ${XCB_XCB_LIBRARY} ${XCB_KEYSYMS_LIBRARY})
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
# KNotify configuration
install( FILES kglobalaccel.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kglobalaccel RENAME kglobalaccel5.notifyrc )

# Install some update file (not yet working)
#install( FILES kconf/kdedglobalaccel_kde42.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)

###
### KDE 4.2 > 4.3 Migration Start
###
### Uninstall the kde 4.0 - 4.2 kdedglobalaccel files
find_file(
    KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE
    kdedglobalaccel.desktop
    PATHS ${SERVICES_INSTALL_DIR}/kded
    NO_DEFAULT_PATH)
find_file(
    KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN
    kded_globalaccel.so
    PATHS ${PLUGIN_INSTALL_DIR}
    NO_DEFAULT_PATH)
if(KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE)
    install(CODE "MESSAGE(\"Removing kdedglobalaccel desktop file\")")
    install(CODE "file(REMOVE ${KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE})")
endif()
if(KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN)
    install(CODE "MESSAGE(\"Removing kdedglobalaccel plugin\")")
    install(CODE "file(REMOVE ${KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN})")
endif()

dbus_add_activation_service(org.kde.kglobalaccel.service.in)
###
### KDE 4.2 > 4.3 Migration End
###

