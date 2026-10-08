add_executable(kfontinst_bin
    ../../dbus/FontinstIface.cpp
    ../../kcmfontinst/FontsPackage.cpp
    ../../kcmfontinst/JobRunner.cpp
    Installer.cpp
)

set_target_properties(kfontinst_bin PROPERTIES OUTPUT_NAME kfontinst)

target_link_libraries(kfontinst_bin
    KF6::Archive
    KF6::IconThemes
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::XmlGui
    KF6::WindowSystem
    X11::X11
    kfontinst
    kfontinstui
    Qt::GuiPrivate
)

install(FILES org.kde.kfontinst.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS kfontinst_bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES installfont.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus)
