plasma_install_package(org.kde.breeze org.kde.breeze.desktop look-and-feel lookandfeel)
plasma_install_package(org.kde.breezedark org.kde.breezedark.desktop look-and-feel lookandfeel)
plasma_install_package(org.kde.breezetwilight org.kde.breezetwilight.desktop look-and-feel lookandfeel)

ecm_add_qml_module(components URI "org.kde.breeze.components" NO_GENERATE_PLUGIN_SOURCE)
ecm_target_qml_sources(components SOURCES
    org.kde.breeze/components/animation/RejectPasswordAnimation.qml
    org.kde.breeze/components/animation/RejectPasswordPathAnimation.qml
    org.kde.breeze/components/ActionButton.qml
    org.kde.breeze/components/Battery.qml
    org.kde.breeze/components/Clock.qml
    org.kde.breeze/components/SessionManagementScreen.qml
    org.kde.breeze/components/UserDelegate.qml
    org.kde.breeze/components/UserList.qml
    org.kde.breeze/components/VirtualKeyboard.qml
    org.kde.breeze/components/VirtualKeyboard_wayland.qml
    org.kde.breeze/components/WallpaperFader.qml
)
target_sources(components PRIVATE org.kde.breeze/components/componentsplugin.cpp)
ecm_finalize_qml_module(components DESTINATION ${KDE_INSTALL_QMLDIR})

qt_add_shaders(components LNFSHADERS
    PRECOMPILE
    BATCHABLE
    OPTIMIZED
    PREFIX "/qt/qml/org/kde/breeze/components/shaders/"
    FILES
        org.kde.breeze/components/UserDelegate.frag
        org.kde.breeze/components/WallpaperFader.frag
    OUTPUTS
        UserDelegate.frag.qsb
        WallpaperFader.frag.qsb
)

if (INSTALL_SDDM_THEME)
    configure_file(sddm-theme/theme.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/sddm-theme/theme.conf)

# Install the login theme into the SDDM directory
    #NOTE this trailing slash is important to rename the directory
    install(DIRECTORY sddm-theme/
        DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze
        PATTERN "README.txt" EXCLUDE
        PATTERN "components" EXCLUDE
        PATTERN "dummydata" EXCLUDE
        PATTERN "theme.conf.cmake" EXCLUDE
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sddm-theme/theme.conf
        DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze
    )
endif()
