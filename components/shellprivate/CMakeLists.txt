ecm_add_qml_module(plasmashellprivateplugin URI "org.kde.plasma.private.shell" VERSION 2.0 GENERATE_PLUGIN_SOURCE)

target_sources(plasmashellprivateplugin PRIVATE
    kcategorizeditemsviewmodels.cpp
    plasmaappletitemmodel.cpp
    widgetexplorer.cpp widgetexplorer.h
)

target_link_libraries(plasmashellprivateplugin PRIVATE
        Qt::Core
        Qt::Quick
        Qt::Qml
        Qt::Gui
        Qt::Widgets
        Qt::Quick
        Qt::Qml
        Plasma::Plasma
        Plasma::PlasmaQuick
        KF6::I18n
        KF6::WidgetsAddons
        KF6::NewStuffWidgets
        KF6::WindowSystem
        Plasma::Activities
        KF6::TextWidgets
        KF6::Package
)

ecm_finalize_qml_module(plasmashellprivateplugin)

install(FILES
    wallpaperplugin.knsrc
    plasmoids.knsrc
    DESTINATION ${KDE_INSTALL_KNSRCDIR}
)
