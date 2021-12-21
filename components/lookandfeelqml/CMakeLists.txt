set(lookandfeelqmlplugin_SRCS
    lookandfeelqmlplugin.cpp
    kpackageinterface.cpp
)

add_library(lookandfeelqmlplugin SHARED ${lookandfeelqmlplugin_SRCS})
target_link_libraries(lookandfeelqmlplugin
    Qt::Qml
    KF5::ConfigCore
    KF5::Package
)

install(TARGETS lookandfeelqmlplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/lookandfeel)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/lookandfeel)
