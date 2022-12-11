set(quickfontviewplugin_SRCS
    quickfontpreviewitem.cpp
    quickfontpreviewplugin.cpp
)

add_library(quickfontviewplugin SHARED ${quickfontviewplugin_SRCS})
target_link_libraries(quickfontviewplugin
    Qt::Core
    Qt::Qml
    Qt::Quick
    KF5::I18n
    KF5::WindowSystem
    Freetype::Freetype
    kfontinstui
)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/fontview)
install(TARGETS quickfontviewplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/fontview)
