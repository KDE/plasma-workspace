
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-shellprivate.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-shellprivate.h)

set(plasmashellprivateplugin_SRCS
    widgetexplorer/kcategorizeditemsviewmodels.cpp
    widgetexplorer/plasmaappletitemmodel.cpp
    widgetexplorer/openwidgetassistant.cpp
    widgetexplorer/widgetexplorer.cpp
    shellprivateplugin.cpp
)

add_library(plasmashellprivateplugin SHARED ${plasmashellprivateplugin_SRCS})
target_link_libraries(plasmashellprivateplugin
        Qt::Core
        Qt::Quick
        Qt::Qml
        Qt::Gui
        Qt::Widgets
        Qt::Quick
        Qt::Qml
        KF6::Plasma
        KF6::PlasmaQuick
        KF6::I18n
        KF6::NewStuff
        KF6::KIOFileWidgets
        KF6::WindowSystem
        KF6::Activities
        KF6::TextWidgets
        KF6::Package
)

install(TARGETS plasmashellprivateplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/shell)
install(FILES
    wallpaperplugin.knsrc
    widgetexplorer/plasmoids.knsrc
    DESTINATION ${KDE_INSTALL_KNSRCDIR}
)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/shell)

