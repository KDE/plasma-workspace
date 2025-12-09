add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.appmenu\")

plasma_add_applet(org.kde.plasma.appmenu
    QML_SOURCES
        qml/configGeneral.qml
        qml/config.qml
        qml/main.qml
        qml/MenuDelegate.qml
    CPP_SOURCES
        appmenuapplet.cpp
        appmenumodel.cpp
    RESOURCES
        main.xml
    QML_ARGS DEPENDENCIES QtCore org.kde.plasma.core
)

target_link_libraries(org.kde.plasma.appmenu PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Quick
    Qt::DBus
    Plasma::Plasma
    KF6::WindowSystem
    KF6::I18n
    PW::LibTaskManager
    dbusmenuqt
)
