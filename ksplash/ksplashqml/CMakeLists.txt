set(ksplashqml_SRCS
    main.cpp
    splashapp.cpp
    splashwindow.cpp
   )

add_executable(ksplashqml ${ksplashqml_SRCS})

target_link_libraries(ksplashqml
    Qt::Quick
    Qt::DBus
    KF5::ConfigCore
    KF5::Package
    KF5::QuickAddons
    KF5::WindowSystem
    PW::KWorkspace
    LayerShellQt::Interface
   )

install(TARGETS ksplashqml ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.KSplash.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

ecm_install_configured_files(INPUT plasma-ksplash.service.in @ONLY DESTINATION  ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
