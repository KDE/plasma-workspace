add_executable(plasma-geoclue-agent)

ecm_qt_declare_logging_category(plasma-geoclue-agent
    HEADER logging.h
    IDENTIFIER AGENT
    CATEGORY_NAME kde.geoclue.agent
    DEFAULT_SEVERITY Info
    DESCRIPTION "geoclue agent"
    EXPORT GEOCLUEAGENT
)

target_compile_definitions(plasma-geoclue-agent PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
)

qt_add_dbus_interface(GENERATED_SOURCES org.freedesktop.GeoClue2.Manager.xml manager)

target_sources(plasma-geoclue-agent PRIVATE
    agent.cpp
    main.cpp
    notifier.cpp
    receiver.cpp

    ${GENERATED_SOURCES}
)

target_link_libraries(plasma-geoclue-agent PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::DBus
    Qt6::Widgets
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    KF6::StatusNotifierItem
)

install(TARGETS plasma-geoclue-agent DESTINATION ${KDE_INSTALL_LIBEXECDIR})
ecm_install_configured_files(INPUT plasma-geoclue-agent.service.in @ONLY DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
