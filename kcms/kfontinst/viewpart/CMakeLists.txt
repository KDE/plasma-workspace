add_library(kfontviewpart MODULE
    ../viewpart/FontPreview.cpp
    ../viewpart/PreviewSelectAction.cpp
    ../viewpart/CharTip.cpp
    ../dbus/FontinstIface.cpp
    FontViewPart.cpp
)

target_link_libraries(kfontviewpart
    KF6::Archive
    KF6::Parts
    KF6::XmlGui
    KF6::WindowSystem
    kfontinstui
    kfontinst
)

install(TARGETS kfontviewpart  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/parts)
install(FILES kfontviewpart.rc DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/kfontviewpart)
