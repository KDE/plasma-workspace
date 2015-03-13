include_directories(
        ${KDEPIMLIBS_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
)

kde_enable_exceptions()

set(calendar_engine_srcs
    calendarengine.cpp
)

if(Akonadi_FOUND)
    add_definitions(-DAKONADI_FOUND)
    set(calendar_engine_srcs ${calendar_engine_srcs}
        eventdatacontainer.cpp
    )
endif()

add_library(plasma_engine_calendar MODULE ${calendar_engine_srcs})

target_link_libraries(
    plasma_engine_calendar
    ${KDEPIMLIBS_KHOLIDAYS_LIBRARY}
    KF5::CoreAddons
    KF5::Plasma
    ${KDE4_KMIME_LIBS}
    KF5::CalendarCore
    ${KDE4_KCALUTILS_LIBS}
)

if(Akonadi_FOUND)
    target_link_libraries(
        plasma_engine_calendar
        akonadi-calendar
        ${KDE4_AKONADI_LIBS}
        ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
    )
endif()

install(TARGETS plasma_engine_calendar DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-calendar.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

