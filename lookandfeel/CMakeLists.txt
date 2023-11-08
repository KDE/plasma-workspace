plasma_install_package(org.kde.breeze org.kde.breeze.desktop look-and-feel lookandfeel)
plasma_install_package(org.kde.breezedark org.kde.breezedark.desktop look-and-feel lookandfeel)
plasma_install_package(org.kde.breezetwilight org.kde.breezetwilight.desktop look-and-feel lookandfeel)

ecm_add_qml_module(components URI "org.kde.breeze.components" NO_GENERATE_PLUGIN_SOURCE)
ecm_target_qml_sources(components SOURCES
    components/animation/RejectPasswordAnimation.qml
    components/animation/RejectPasswordPathAnimation.qml
    components/ActionButton.qml
    components/Battery.qml
    components/Clock.qml
    components/SessionManagementScreen.qml
    components/UserDelegate.qml
    components/UserList.qml
    components/VirtualKeyboard.qml
    components/VirtualKeyboard_wayland.qml
    components/WallpaperFader.qml
)
target_sources(components PRIVATE components/componentsplugin.cpp)
ecm_finalize_qml_module(components DESTINATION ${KDE_INSTALL_QMLDIR})

qt_add_shaders(components LNFSHADERS
    PRECOMPILE
    BATCHABLE
    OPTIMIZED
    PREFIX "/qt/qml/org/kde/breeze/components/shaders/"
    FILES
        components/UserDelegate.frag
        components/WallpaperFader.frag
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
