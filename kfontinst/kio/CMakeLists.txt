kcoreaddons_add_plugin(kio_fonts SOURCES FontInstInterface.cpp KioFonts.cpp ../dbus/FontinstIface.cpp ${KFI_FONTINST_AUTH_SRC} INSTALL_NAMESPACE "kf6/kio")

ecm_qt_declare_logging_category(kio_fonts
    HEADER debug.h
    IDENTIFIER KCM_KFONTINST_KIO
    CATEGORY_NAME org.kde.kcm_kfontinst.kio
    DESCRIPTION "kcm kfontinst kio"
    EXPORT PLASMAWORKSPACE
)

target_link_libraries(kio_fonts
    Qt::DBus
    Qt::Xml
    KF6::Archive
    KF6::KIOCore
    KF6::KIOWidgets
    kfontinst
    Qt::GuiPrivate
)

install(FILES fonts.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqsidebartng/virtual_folders/services)
ecm_install_icons( ICONS
    128-mimetypes-fonts-package.png
    16-mimetypes-fonts-package.png
    22-mimetypes-fonts-package.png
    32-mimetypes-fonts-package.png
    48-mimetypes-fonts-package.png
    64-mimetypes-fonts-package.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor
)
