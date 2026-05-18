ecm_add_qml_module(calendarplugin
    URI "org.kde.plasma.workspace.calendar"
    VERSION 2.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtCore
)

target_sources(calendarplugin PRIVATE
    calendar.cpp calendar.h
    daysmodel.cpp daysmodel.h
    eventdatadecorator.cpp eventdatadecorator.h
    eventpluginsmanager.cpp eventpluginsmanager.h
)

ecm_target_qml_sources(calendarplugin
    SOURCES
    qml/MonthView.qml
)

ecm_target_qml_sources(calendarplugin
    PRIVATE
    SOURCES
    qml/DayDelegate.qml
    qml/DaysCalendar.qml
    qml/InfiniteList.qml
    qml/MonthViewHeader.qml
)

ecm_qt_declare_logging_category(calendarplugin
    HEADER debug_p.h
    IDENTIFIER "COMPONENTS::CALENDAR"
    CATEGORY_NAME org.kde.plasma.workspace.calendar
    DESCRIPTION "Plasma Calendar component"
    DEFAULT_SEVERITY Warning
    EXPORT "COMPONENTS::CALENDAR"
)

target_link_libraries(calendarplugin PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    KF6::I18n
    KF6::CalendarEvents
    KF6::CoreAddons
)

ecm_finalize_qml_module(calendarplugin)

set(QTPATHS_PATH "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qtpaths>")
configure_file(kconf_update/migrate-calendar-to-plugin-id.py.in ${CMAKE_CURRENT_BINARY_DIR}/migrate-calendar-to-plugin-id.py.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/migrate-calendar-to-plugin-id.py INPUT ${CMAKE_CURRENT_BINARY_DIR}/migrate-calendar-to-plugin-id.py.in)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/migrate-calendar-to-plugin-id.py DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(FILES kconf_update/migrate-calendar-to-plugin-id.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
