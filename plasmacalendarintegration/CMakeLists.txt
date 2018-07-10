add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

set(holidays-plugin_SRCS
    holidaysevents.cpp
)

add_library(holidaysevents MODULE ${holidays-plugin_SRCS})
target_link_libraries(holidaysevents Qt5::Core
                                     KF5::ConfigCore
                                     KF5::Holidays
                                     KF5::CalendarEvents
)

install (TARGETS holidaysevents
         DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasmacalendarplugins
)

install (FILES HolidaysConfig.qml
         DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasmacalendarplugins/holidays
)

add_subdirectory(qmlhelper)
