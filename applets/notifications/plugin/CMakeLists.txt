project(notificationshelper)

set(notificationshelper_SRCS
    notificationshelper.cpp
    notificationshelperplugin.cpp
    )

add_library(notificationshelperplugin SHARED ${notificationshelper_SRCS})
target_link_libraries(notificationshelperplugin Qt5::Core
                                                Qt5::Gui
                                                Qt5::Qml
                                                Qt5::Quick
                                                KF5::WindowSystem)

install(TARGETS notificationshelperplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/notifications)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/notifications)