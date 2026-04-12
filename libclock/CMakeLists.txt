ecm_add_qml_module(clockplugin
URI "org.kde.plasma.clock"
GENERATE_PLUGIN_SOURCE
)

if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    target_sources(clockplugin PRIVATE alignedtimer_posix.cpp clock.cpp)
else()
    target_sources(clockplugin PRIVATE alignedtimer.cpp clock.cpp)
endif()

target_link_libraries(clockplugin
    PRIVATE
        Qt::Core
        Qt::Qml
        Qt::DBus
)

ecm_finalize_qml_module(clockplugin)
