kcoreaddons_add_plugin(plasma_containmentactions_paste SOURCES paste.cpp INSTALL_NAMESPACE "plasma/containmentactions")
target_link_libraries(plasma_containmentactions_paste
    Qt::Gui
    Qt::Widgets
    KF5::Plasma
    KF5::KIOCore
)

kcoreaddons_desktop_to_json(plasma_containmentactions_paste plasma-containmentactions-paste.desktop)
