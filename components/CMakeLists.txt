project(plasmashellprivateplugin)

add_definitions(-DTRANSLATION_DOMAIN=\"widgetexplorer\")

if (KF5TextEditor_FOUND)
   set(interactiveconsole_SRCS
       interactiveconsole/interactiveconsole.cpp
       )
endif (KF5TextEditor_FOUND)


set(plasmashellprivateplugin_SRCS
    widgetexplorer/kcategorizeditemsviewmodels.cpp
    widgetexplorer/plasmaappletitemmodel.cpp
    widgetexplorer/openwidgetassistant.cpp
    widgetexplorer/widgetexplorer.cpp
    shellprivateplugin.cpp
    ${interactiveconsole_SRCS}
)

add_library(plasmashellprivateplugin SHARED ${plasmashellprivateplugin_SRCS})
target_link_libraries(plasmashellprivateplugin
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

if (KF5TextEditor_FOUND)
   target_link_libraries(plasmashellprivateplugin
                         KF5::TextEditor)
endif (KF5TextEditor_FOUND)

install(TARGETS plasmashellprivateplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/shell)
install(FILES widgetexplorer/plasmoids.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/shell)
install(DIRECTORY qml/ DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/workspace/components)

