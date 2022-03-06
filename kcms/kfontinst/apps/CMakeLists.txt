set(kfontinst_bin_SRCS ${libkfontinstjobrunner_SRCS} ${libkfontinstdbusiface_SRCS} Installer.cpp )
# qt_add_dbus_interface(kfontinst_bin_SRCS ../dbus/org.kde.fontinst.xml FontInstInterfaceBase)

set(kfontprint_bin_SRCS ${libkfontinstactionlabel_SRCS} Printer.cpp )
set(kfontview_bin_SRCS Viewer.cpp )

add_executable(kfontinst_bin ${kfontinst_bin_SRCS})
add_executable(kfontprint_bin ${kfontprint_bin_SRCS})
add_executable(kfontview_bin ${kfontview_bin_SRCS})

set_target_properties(kfontinst_bin PROPERTIES OUTPUT_NAME kfontinst)
set_target_properties(kfontprint_bin PROPERTIES OUTPUT_NAME kfontprint)
set_target_properties(kfontview_bin PROPERTIES OUTPUT_NAME kfontview)

target_link_libraries(kfontinst_bin
    KF5::Archive
    KF5::IconThemes
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::XmlGui
    X11::X11
    kfontinst
)
if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(kfontinst_bin Qt::X11Extras)
else()
    target_link_libraries(kfontinst_bin Qt::GuiPrivate)
endif()
target_link_libraries(kfontprint_bin
    Qt::PrintSupport
    KF5::IconThemes
    kfontinstui
    kfontinst
)
target_link_libraries(kfontview_bin KF5::Parts KF5::XmlGui KF5::DBusAddons kfontinstui kfontinst )


ecm_qt_declare_logging_category(kfontview_bin
    HEADER kfontview_debug.h
    IDENTIFIER KFONTVIEW_DEBUG
    CATEGORY_NAME org.kde.plasma.kfontview
)

install(TARGETS kfontinst_bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install(TARGETS kfontprint_bin DESTINATION ${KDE_INSTALL_LIBEXECDIR} )
install(TARGETS kfontview_bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES kfontviewui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kfontview )
install(PROGRAMS org.kde.kfontview.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install(FILES installfont.desktop DESTINATION
${KDE_INSTALL_KSERVICESDIR}/ServiceMenus )

ecm_install_icons( ICONS
                   16-apps-kfontview.png
                   22-apps-kfontview.png
                   32-apps-kfontview.png
                   48-apps-kfontview.png
                   64-apps-kfontview.png
                   DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
