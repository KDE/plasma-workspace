project(plasmawidgetexplorerplugin)

add_definitions(-DTRANSLATION_DOMAIN=\"widgetexplorer\")

set(widgetexplorer_SRC
    kcategorizeditemsviewmodels.cpp
    plasmaappletitemmodel.cpp
    openwidgetassistant.cpp
    widgetexplorer.cpp
    widgetexplorerplugin.cpp
)

add_library(plasmawidgetexplorerplugin SHARED ${widgetexplorer_SRC})
target_link_libraries(plasmawidgetexplorerplugin
        Qt5::Core
        Qt5::Quick
        Qt5::Qml
        Qt5::Gui
        Qt5::Widgets
        Qt5::Quick
        Qt5::Qml
        KF5::Plasma
        KF5::PlasmaQuick
        KF5::I18n
        KF5::Service
        KF5::NewStuff
        KF5::KIOFileWidgets
        KF5::WindowSystem
)

install(TARGETS plasmawidgetexplorerplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/shell)
install(FILES plasmoids.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/shell)
