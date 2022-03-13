add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_time\")

set(time_engine_SRCS
    timeengine.cpp
    timesource.cpp
    solarsystem.cpp
    timeengine.h
    timesource.h
    solarsystem.h
)

ecm_qt_declare_logging_category(time_engine_SRCS HEADER debug.h
                                               IDENTIFIER DATAENGINE_TIME
                                               CATEGORY_NAME kde.dataengine.time
                                               DEFAULT_SEVERITY Info)

kcoreaddons_add_plugin(plasma_engine_time SOURCES ${time_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_time
    Qt::DBus
    KF5::Solid
    KF5::Plasma
    KF5::I18n
    KF5::Service
)
