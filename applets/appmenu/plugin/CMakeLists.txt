ecm_add_qml_module(appmenuplugin
    URI "org.kde.plasma.private.appmenu"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtCore
)

target_sources(appmenuplugin PRIVATE
    appmenumodel.cpp
    appmenumodel.h
)

target_link_libraries(appmenuplugin PRIVATE
                      Qt::Core
                      Qt::Widgets
                      Qt::Quick
                      Plasma::Plasma
                      KF6::WindowSystem
                      KF6::I18n
                      PW::LibTaskManager
                      dbusmenuqt)

ecm_finalize_qml_module(appmenuplugin)
