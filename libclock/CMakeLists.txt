ecm_add_qml_module(clockplugin
URI "org.kde.plasma.clock"
GENERATE_PLUGIN_SOURCE
)

target_sources(clockplugin PRIVATE alignedtimer.cpp clock.cpp)

target_link_libraries(clockplugin
    PRIVATE
        Qt::Core
        Qt::Qml
        Qt::DBus
)

ecm_finalize_qml_module(clockplugin)
