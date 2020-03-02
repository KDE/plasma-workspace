add_subdirectory(plasma-autostart-list)

set(plasma_session_SRCS
    main.cpp
    autostart.cpp
    startup.cpp
)

ecm_qt_declare_logging_category(plasma_session_SRCS  HEADER debug.h IDENTIFIER PLASMA_SESSION CATEGORY_NAME org.kde.plasma.session)

qt5_add_dbus_adaptor( plasma_session_SRCS org.kde.Startup.xml startup.h Startup)

set(kcminit_adaptor ${plasma-workspace_SOURCE_DIR}/startkde/kcminit/main.h)
set(kcminit_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KCMinit.xml)
qt5_generate_dbus_interface( ${kcminit_adaptor} ${kcminit_xml} )
qt5_add_dbus_interface( plasma_session_SRCS ${kcminit_xml} kcminit_interface )
qt5_add_dbus_interface( plasma_session_SRCS ${KDED_DBUS_INTERFACE} kded_interface )

set(klauncher_xml ${KINIT_DBUS_INTERFACES_DIR}/kf5_org.kde.KLauncher.xml)
qt5_add_dbus_interface( plasma_session_SRCS ${klauncher_xml} klauncher_interface )

qt5_add_dbus_interface( plasma_session_SRCS ../../ksmserver/org.kde.KSMServerInterface.xml ksmserver_interface )


add_executable(plasma_session ${plasma_session_SRCS})

target_link_libraries(plasma_session
    Qt5::Core
    Qt5::DBus
    KF5::ConfigCore
    KF5::Service
    KF5::CoreAddons
    KF5::Notifications
    KF5::KIOCore
    PW::KWorkspace
    ${PHONON_LIBRARIES}
)

install(TARGETS plasma_session ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

