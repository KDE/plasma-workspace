set(systemtraycontainer_SRCS
    systemtraycontainer.cpp
    systemtraycontainer.h
)

ecm_qt_declare_logging_category(systemtraycontainer_SRCS HEADER debug.h
                                            IDENTIFIER SYSTEM_TRAY_CONTAINER
                                            CATEGORY_NAME kde.systemtraycontainer
                                            DEFAULT_SEVERITY Info
                                            DESCRIPTION "plasma applet systemtray container"
                                            EXPORT PLASMAWORKSPACE
                                        )

plasma_add_applet(org.kde.plasma.systemtray
    CPP_SOURCES ${systemtraycontainer_SRCS}
    QML_SOURCES main.qml
)

target_link_libraries(org.kde.plasma.systemtray PRIVATE
                      Qt::Gui
                      Qt::Quick
                      Plasma::Plasma
                      Plasma::PlasmaQuick
                      KF6::XmlGui
                      KF6::I18n)
