ecm_add_qml_module(brightnesscontrolplugin URI org.kde.plasma.private.brightnesscontrolplugin)
target_sources(brightnesscontrolplugin PRIVATE
    nightcolorinhibitor.cpp
    nightcolormonitor.cpp
    plugin.cpp
)
target_link_libraries(brightnesscontrolplugin PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Qml
)
ecm_finalize_qml_module(brightnesscontrolplugin)
