add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.systemtray\")

set(systemtrayplugin_SRCS
    systemtrayplugin.cpp
    host.cpp
   # manager.cpp
    protocol.cpp
    task.cpp
    tasklistmodel.cpp
    tasksproxymodel.cpp

    protocols/dbussystemtray/dbussystemtraytask.cpp
    protocols/dbussystemtray/dbussystemtrayprotocol.cpp

    protocols/plasmoid/plasmoidtask.cpp
    protocols/plasmoid/plasmoidprotocol.cpp
)

add_library(systemtrayplugin SHARED ${systemtrayplugin_SRCS})

target_link_libraries(systemtrayplugin
    Qt5::DBus
    Qt5::Quick
    Qt5::Qml
    KF5::Declarative
    KF5::Plasma
    KF5::PlasmaQuick
    KF5::I18n
    KF5::IconThemes
    KF5::KIOCore
    KF5::XmlGui #for KActionCollection
    )

set(plugin_install_path ${KDE_INSTALL_QMLDIR}/org/kde/private/systemtray)

install(TARGETS systemtrayplugin DESTINATION ${plugin_install_path})
install(FILES qmldir DESTINATION ${plugin_install_path})
