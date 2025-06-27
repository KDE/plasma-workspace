plasma_add_applet(org.kde.plasma.panelspacer
    QML_SOURCES
        main.qml
    CPP_SOURCES
        panelspacer.cpp
    RESOURCES
        main.xml
)

target_link_libraries(org.kde.plasma.panelspacer PRIVATE Qt::Gui Qt::Core Qt::Qml Qt::Quick Plasma::Plasma Plasma::PlasmaQuick KF6::I18n)
