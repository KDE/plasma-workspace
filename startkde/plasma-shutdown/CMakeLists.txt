set(plasma_shutdown_SRCS
    main.cpp
    shutdown.cpp
)

ecm_qt_declare_logging_category(plasma_shutdown_SRCS  HEADER debug.h IDENTIFIER PLASMA_SESSION CATEGORY_NAME org.kde.plasma.shutdown)

qt5_add_dbus_adaptor(plasma_shutdown_SRCS org.kde.Shutdown.xml shutdown.h Shutdown)
qt5_add_dbus_interface(plasma_shutdown_SRCS org.kde.Shutdown.xml shutdown_interface)
qt5_add_dbus_interface( plasma_shutdown_SRCS ../../ksmserver/org.kde.KSMServerInterface.xml ksmserver_interface )

add_executable(plasma-shutdown ${plasma_shutdown_SRCS})

target_link_libraries(plasma-shutdown
    Qt5::Core
    Qt5::DBus
    KF5::ConfigCore
    PW::KWorkspace
)

kdbusaddons_generate_dbus_service_file(plasma-shutdown org.kde.Shutdown ${KDE_INSTALL_FULL_BINDIR})
install(TARGETS plasma-shutdown ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

