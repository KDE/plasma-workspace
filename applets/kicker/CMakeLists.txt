add_definitions(
  -DTRANSLATION_DOMAIN=\"libkicker\"
)

set(kickerplugin_SRCS
    abstractentry.cpp
    abstractmodel.cpp
    actionlist.cpp
    appentry.cpp
    appsmodel.cpp
    computermodel.cpp
    containmentinterface.cpp
    draghelper.cpp
    simplefavoritesmodel.cpp
    kastatsfavoritesmodel.cpp
    fileentry.cpp
    forwardingmodel.cpp
    placeholdermodel.cpp
    funnelmodel.cpp
    dashboardwindow.cpp
    processrunner.cpp
    rootmodel.cpp
    runnermodel.cpp
    runnermatchesmodel.cpp
    recentusagemodel.cpp
    submenu.cpp
    systementry.cpp
    systemmodel.cpp
    systemsettings.cpp
    wheelinterceptor.cpp
    windowsystem.cpp
    funnelmodel.cpp
    sectionsmodel.cpp
)

ecm_qt_declare_logging_category(kickerplugin_SRCS
    HEADER debug.h
    IDENTIFIER KICKER_DEBUG
    CATEGORY_NAME org.kde.plasma.kicker
    DESCRIPTION "kicker debug"
    EXPORT PLASMAWORKSPACE
    )

qt_add_dbus_interface(kickerplugin_SRCS ${CMAKE_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml krunner_interface)
qt_add_dbus_interface(kickerplugin_SRCS ${CMAKE_SOURCE_DIR}/ksmserver/org.kde.KSMServerInterface.xml ksmserver_interface)

ecm_add_qml_module(kickerplugin
    URI "org.kde.plasma.private.kicker"
    VERSION 0.1
    SOURCES ${kickerplugin_SRCS}
    DEPENDENCIES QtCore QtQuick
    GENERATE_PLUGIN_SOURCE
)

if (BUILD_TESTING)
    set_target_properties(kickerplugin PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

target_link_libraries(kickerplugin PRIVATE
                      Qt::Core
                      Qt::Qml
                      Qt::Quick
                      Plasma::Activities
                      Plasma::ActivitiesStats
                      KF6::ConfigCore
                      KF6::CoreAddons
                      KF6::I18n
                      KF6::IconThemes
                      KF6::ItemModels
                      KF6::KIOCore
                      KF6::KIOWidgets
                      KF6::KIOFileWidgets
                      KF6::Notifications
                      Plasma::PlasmaQuick
                      KF6::Runner
                      KF6::Service
                      KF6::WindowSystem
                      Wayland::Client
                      PW::KWorkspace)

if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
    set(private_code_option  "PRIVATE_CODE")
endif()
qt_generate_wayland_protocol_client_sources(kickerplugin
    FILES
        "${PLASMA_WAYLAND_PROTOCOLS_DIR}/plasma-shell.xml"
    ${private_code_option}
)

if (${HAVE_APPSTREAMQT})
target_link_libraries(kickerplugin PRIVATE AppStreamQt)
endif()

if (${HAVE_ICU})
    target_link_libraries(kickerplugin PRIVATE ICU::i18n ICU::uc)
    target_compile_definitions(kickerplugin PRIVATE "-DHAVE_ICU")
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

ecm_finalize_qml_module(kickerplugin)
