add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.systemtray\")

include_directories(${plasma-workspace_SOURCE_DIR}/statusnotifierwatcher)

set(systemtray_SRCS
    dbusserviceobserver.cpp dbusserviceobserver.h
    plasmoidregistry.cpp plasmoidregistry.h
    statusnotifieritemhost.cpp statusnotifieritemhost.h
    statusnotifieritemsource.cpp statusnotifieritemsource.h
    systemtraysettings.cpp systemtraysettings.h
    systemtraytypes.cpp
    systemtraymodel.cpp
    sortedsystemtraymodel.cpp sortedsystemtraymodel.h
)

qt_add_dbus_interface(systemtray_SRCS ${KSTATUSNOTIFIERITEM_DBUS_INTERFACES_DIR}/kf6_org.kde.StatusNotifierWatcher.xml statusnotifierwatcher_interface)
qt_add_dbus_interface(systemtray_SRCS ${plasma-workspace_SOURCE_DIR}/libkmpris/dbus/org.freedesktop.DBus.Properties.xml dbusproperties)

set(statusnotifieritem_xml ${KSTATUSNOTIFIERITEM_DBUS_INTERFACES_DIR}/kf6_org.kde.StatusNotifierItem.xml)
set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtraytypes.h"
   CLASSNAME OrgKdeStatusNotifierItem
)
qt_add_dbus_interface(systemtray_SRCS ${statusnotifieritem_xml} statusnotifieritem_interface)

ecm_qt_declare_logging_category(systemtray_SRCS HEADER debug.h
                                            IDENTIFIER SYSTEM_TRAY
                                            CATEGORY_NAME kde.systemtray
                                            DEFAULT_SEVERITY Info
                                            DESCRIPTION "plasma applet systemtray"
                                            EXPORT PLASMAWORKSPACE
                                        )
add_library(systemtray_static STATIC ${systemtray_SRCS})
target_link_libraries(systemtray_static
    PUBLIC
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
        Qt::Quick
        KF6::ConfigCore
        KF6::IconThemes
        KF6::I18n
        KF6::KIOGui
        KF6::JobWidgets
        KF6::Service
        KF6::ItemModels
        KF6::WindowSystem
        KF6::XmlGui
        Plasma::Plasma
        Plasma::PlasmaQuick
        dbusmenuqt
)

set_property(TARGET systemtray_static PROPERTY POSITION_INDEPENDENT_CODE ON)

plasma_add_applet(org.kde.plasma.systemtray
    CPP_SOURCES
        systemtray.cpp
    QML_SOURCES
        qml/AbstractItem.qml
        qml/ConfigGeneral.qml
        qml/CurrentItemHighLight.qml
        qml/ExpanderArrow.qml
        qml/ItemLoader.qml
        qml/PlasmoidItem.qml
        qml/PulseAnimation.qml
        qml/SystemTrayState.qml
        qml/CompactApplet.qml
        qml/config.qml
        qml/ExpandedRepresentation.qml
        qml/HiddenItemsView.qml
        qml/main.qml
        qml/PlasmoidPopupsContainer.qml
        qml/StatusNotifierItem.qml
    RESOURCES
        main.xml
)

target_link_libraries(org.kde.plasma.systemtray PRIVATE systemtray_static)

install(FILES statusnotifieritem.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)

if(BUILD_TESTING)
  add_subdirectory(autotests)
  add_subdirectory(tests)
endif()
