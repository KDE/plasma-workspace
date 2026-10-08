add_subdirectory(kfontinst)
add_subdirectory(kfontprint)
add_subdirectory(kfontview)

ecm_install_icons( ICONS
    16-apps-kfontview.png
    22-apps-kfontview.png
    32-apps-kfontview.png
    48-apps-kfontview.png
    64-apps-kfontview.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor
)
