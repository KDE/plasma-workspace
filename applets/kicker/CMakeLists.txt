add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_CAST_FROM_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_USE_FAST_OPERATOR_PLUS
  -DTRANSLATION_DOMAIN=\"libkicker\"
)

set(kickerplugin_SRCS
    plugin/abstractentry.cpp
    plugin/abstractmodel.cpp
    plugin/actionlist.cpp
    plugin/appentry.cpp
    plugin/appsmodel.cpp
    plugin/computermodel.cpp
    plugin/containmentinterface.cpp
    plugin/draghelper.cpp
    plugin/simplefavoritesmodel.cpp
    plugin/kastatsfavoritesmodel.cpp
    plugin/fileentry.cpp
    plugin/forwardingmodel.cpp
    plugin/placeholdermodel.cpp
    plugin/funnelmodel.cpp
    plugin/dashboardwindow.cpp
    plugin/kickerplugin.cpp
    plugin/menuentryeditor.cpp
    plugin/processrunner.cpp
    plugin/rootmodel.cpp
    plugin/runnermodel.cpp
    plugin/runnermatchesmodel.cpp
    plugin/recentusagemodel.cpp
    plugin/submenu.cpp
    plugin/systementry.cpp
    plugin/systemmodel.cpp
    plugin/systemsettings.cpp
    plugin/wheelinterceptor.cpp
    plugin/windowsystem.cpp
    plugin/funnelmodel.cpp
    plugin/sectionsmodel.cpp
    plugin/kickercompattrianglemousefilter.cpp
    ../../components/trianglemousefilter/trianglemousefilter.cpp
)

ecm_qt_declare_logging_category(kickerplugin_SRCS
    HEADER debug.h
    IDENTIFIER KICKER_DEBUG
    CATEGORY_NAME org.kde.plasma.kicker)

qt_add_dbus_interface(kickerplugin_SRCS ${CMAKE_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml krunner_interface)
qt_add_dbus_interface(kickerplugin_SRCS ${CMAKE_SOURCE_DIR}/ksmserver/org.kde.KSMServerInterface.xml ksmserver_interface)

install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/kicker)

add_library(kickerplugin SHARED ${kickerplugin_SRCS})

if (BUILD_TESTING)
    set_target_properties(kickerplugin PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

target_link_libraries(kickerplugin
                      Qt::Core
                      Qt::Qml
                      Qt::Quick
                      KF6::Activities
                      KF6::ActivitiesStats
                      KF6::ConfigCore
                      KF6::CoreAddons
                      KF6::I18n
                      KF6::IconThemes
                      KF6::ItemModels
                      KF6::KIOCore
                      KF6::KIOWidgets
                      KF6::KIOFileWidgets
                      KF6::Notifications
                      KF6::PlasmaQuick
                      KF6::Runner
                      KF6::Service
                      KF6::WindowSystem
                      KF6::XmlGui
                      PW::KWorkspace)

if (${HAVE_APPSTREAMQT})
target_link_libraries(kickerplugin AppStreamQt)
endif()

if (${HAVE_ICU})
    target_link_libraries(kickerplugin ICU::i18n ICU::uc)
    target_compile_definitions(kickerplugin PRIVATE "-DHAVE_ICU")
endif()

if (BUILD_TESTING)
    add_subdirectory(plugin/autotests)
endif()

install(TARGETS kickerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/kicker)
