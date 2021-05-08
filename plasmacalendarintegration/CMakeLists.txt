add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

kcoreaddons_add_plugin(holidaysevents SOURCES holidaysevents.cpp INSTALL_NAMESPACE "plasmacalendarplugins")
target_link_libraries(holidaysevents Qt::Core
                                     KF5::ConfigCore
                                     KF5::Holidays
                                     KF5::CalendarEvents
)

install (FILES HolidaysConfig.qml
         DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasmacalendarplugins/holidays
)

add_subdirectory(qmlhelper)
