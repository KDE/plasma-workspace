set(notificationsapplet_SRCS
    notificationsapplet.cpp
)

add_library(plasma_applet_notifications MODULE ${notificationsapplet_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_notifications ../package/metadata.desktop)

target_link_libraries(plasma_applet_notifications
                      KF5::WindowSystem
                      KF5::Plasma
                      KF5::ConfigCore)

install(TARGETS plasma_applet_notifications DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/applets)
