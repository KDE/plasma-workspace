set(paste_SRCS
    paste.cpp
)

add_library(plasma_containmentactions_paste MODULE ${paste_SRCS})
target_link_libraries(plasma_containmentactions_paste
    Qt5::Gui
    Qt5::Widgets
    KF5::Plasma
    KF5::KIOCore
    )

kcoreaddons_desktop_to_json(plasma_containmentactions_paste plasma-containmentactions-paste.desktop)

install(TARGETS plasma_containmentactions_paste DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-containmentactions-paste.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
