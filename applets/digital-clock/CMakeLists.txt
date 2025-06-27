add_subdirectory(plugin)

plasma_add_applet(org.kde.plasma.digitalclock
    QML_SOURCES
        CalendarView.qml
        configAppearance.qml
        configCalendar.qml
        configTimeZones.qml
        DigitalClock.qml
        main.qml
        NoTimezoneWarning.qml
        Tooltip.qml
        config.qml
    RESOURCES
        main.xml
    GENERATE_APPLET_CLASS
)
