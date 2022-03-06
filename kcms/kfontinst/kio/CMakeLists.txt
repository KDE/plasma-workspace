set(kio_fonts_PART_SRCS FontInstInterface.cpp KioFonts.cpp ${libkfontinstdbusiface_SRCS})
ecm_qt_declare_logging_category(kio_fonts_PART_SRCS
    HEADER debug.h
    IDENTIFIER KCM_KFONTINST_KIO
    CATEGORY_NAME org.kde.kcm_kfontinst.kio)
# qt_add_dbus_interface(kio_fonts_PART_SRCS ../dbus/org.kde.fontinst.xml FontinstIface)
kcoreaddons_add_plugin(kio_fonts SOURCES ${kio_fonts_PART_SRCS} ${KFI_FONTINST_AUTH_SRC} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kio")
target_link_libraries(kio_fonts Qt::DBus Qt::Xml KF5::Archive KF5::KIOCore KF5::KIOWidgets kfontinst )
if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(kio_fonts Qt::X11Extras)
else()
    target_link_libraries(kio_fonts Qt::GuiPrivate)
endif()

install( FILES fonts.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqsidebartng/virtual_folders/services )
ecm_install_icons( ICONS
                   128-mimetypes-fonts-package.png
                   16-mimetypes-fonts-package.png
                   22-mimetypes-fonts-package.png
                   32-mimetypes-fonts-package.png
                   48-mimetypes-fonts-package.png
                   64-mimetypes-fonts-package.png
                   DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor )
