set(quickfontviewplugin_SRCS
    quickfontpreviewbackend.cpp
    quickfontpreviewplugin.cpp
)

add_library(quickfontviewplugin SHARED ${quickfontviewplugin_SRCS})
target_link_libraries(quickfontviewplugin
    Qt::Core
    Qt::Qml
    Qt::Quick
    Fontconfig::Fontconfig
    Freetype::Freetype
    KF5::I18n
)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/fontview)
install(TARGETS quickfontviewplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/fontview)
