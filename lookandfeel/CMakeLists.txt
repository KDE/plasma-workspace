plasma_install_package(org.kde.breeze org.kde.breeze.desktop look-and-feel lookandfeel)
plasma_install_package(org.kde.breezedark org.kde.breezedark.desktop look-and-feel lookandfeel)
plasma_install_package(org.kde.breezetwilight org.kde.breezetwilight.desktop look-and-feel lookandfeel)


add_custom_target(dummytargetforqsb ALL)
qt_add_shaders(dummytargetforqsb LNFSHADERS
    PRECOMPILE
    BATCHABLE
    OPTIMIZED
    FILES
        org.kde.breeze/contents/components/UserDelegate.frag
        org.kde.breeze/contents/components/WallpaperFader.frag
    OUTPUTS
        UserDelegate.frag.qsb
        WallpaperFader.frag.qsb
)
set(QSB_OUTPUT_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/.qsb/UserDelegate.frag.qsb
    ${CMAKE_CURRENT_BINARY_DIR}/.qsb/WallpaperFader.frag.qsb
)

install(FILES ${QSB_OUTPUT_FILES}
    DESTINATION ${PLASMA_DATA_INSTALL_DIR}/look-and-feel/org.kde.breeze.desktop/contents/components
)

if (INSTALL_SDDM_THEME)
    configure_file(sddm-theme/theme.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/sddm-theme/theme.conf)

# Install the login theme into the SDDM directory
# Longer term we need to look at making SDDM load from look and feel somehow.. and allow copying at runtime
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

    install(DIRECTORY org.kde.breeze/contents/components
        DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze
        PATTERN "README.txt" EXCLUDE
    )
    install(FILES ${QSB_OUTPUT_FILES}
            DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze/components
    )
endif()
