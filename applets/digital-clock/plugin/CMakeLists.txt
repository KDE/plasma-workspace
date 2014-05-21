project(processrunnerplugin)

set(processrunnerplugin_SRCS
    processrunner.cpp
    processrunnerplugin.cpp
    )

add_library(processrunnerplugin SHARED ${processrunnerplugin_SRCS})
target_link_libraries(processrunnerplugin Qt5::Core
                                          Qt5::Qml
                                          KF5::CoreAddons)

install(TARGETS processrunnerplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/digitalclock)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/digitalclock)