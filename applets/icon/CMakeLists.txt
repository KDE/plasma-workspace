add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.icon\")

plasma_add_applet(org.kde.plasma.icon
    CPP_SOURCES
        iconapplet.cpp
    QML_SOURCES
        qml/main.qml
    RESOURCES
        main.xml
)

target_link_libraries(org.kde.plasma.icon PRIVATE
    KF6::I18n
    KF6::KIOCore # for OpenFileManagerWindowJob
    KF6::KIOGui # for FavIconRequestJob
    KF6::KIOWidgets # for KRun
    KF6::Notifications
    Plasma::Plasma
    PW::LibTaskManager
)
