add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_time\")

set(time_engine_SRCS
    timeengine.cpp
    timesource.cpp
    solarsystem.cpp
)

ecm_qt_declare_logging_category(time_engine_SRCS HEADER debug.h
                                               IDENTIFIER DATAENGINE_TIME
                                               CATEGORY_NAME kde.dataengine.time
                                               DEFAULT_SEVERITY Info)

add_library(plasma_engine_time MODULE ${time_engine_SRCS})

target_link_libraries(plasma_engine_time
    Qt::DBus
    KF5::Solid
    KF5::Plasma
    KF5::I18n
    KF5::Service
)

kcoreaddons_desktop_to_json(plasma_engine_time plasma-dataengine-time.desktop)

install(TARGETS plasma_engine_time DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-time.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
