add_library(calendarplugin SHARED)

target_sources(calendarplugin PRIVATE
    calendarplugin.cpp
    calendar.cpp
    daysmodel.cpp
    eventdatadecorator.cpp
    eventpluginsmanager.cpp
)

target_link_libraries(calendarplugin
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    KF6::I18n
    KF6::CalendarEvents
    KF6::CoreAddons
)

install(TARGETS calendarplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/workspace/calendar)
install(DIRECTORY qml/ DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/workspace/calendar)

ecm_generate_qmltypes(org.kde.plasma.workspace.calendar 2.0 DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/workspace/calendar)

install(PROGRAMS kconf_update/migrate-calendar-to-plugin-id.py DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(FILES kconf_update/migrate-calendar-to-plugin-id.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
