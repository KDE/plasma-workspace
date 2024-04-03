ecm_add_qml_module(holidayevents
    URI "org.kde.plasma.private.holidayevents"
    GENERATE_PLUGIN_SOURCE
)

target_sources(holidayevents
    PRIVATE
        holidayregionsconfig.h
        holidayregionsconfig.cpp
)

target_link_libraries(holidayevents
    PRIVATE
        Qt::Qml
        Qt::Core
        KF6::ConfigCore
)

ecm_finalize_qml_module(holidayevents
    DESTINATION ${KDE_INSTALL_QMLDIR}
)
