project(ksplashqml)

add_subdirectory(themes)

set(ksplashqml_SRCS
    main.cpp
    SplashApp.cpp
    SplashWindow.cpp
   )

add_executable(ksplashqml ${ksplashqml_SRCS})

target_link_libraries(ksplashqml
    Qt5::Quick
    Qt5::DBus
    KF5::ConfigCore
    KF5::Package
   )

install(TARGETS ksplashqml ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.KSplash.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

