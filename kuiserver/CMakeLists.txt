add_definitions(-DTRANSLATION_DOMAIN=\"kuiserver5\")

add_subdirectory( tests )

set(kuiserver_KDEINIT_SRCS
    main.cpp
    uiserver.cpp
    jobview.cpp
    progresslistmodel.cpp
    progresslistdelegate.cpp
    requestviewcallwatcher.cpp
)

set(kuiserver_KDEINIT_PRIVATE
progresslistdelegate_p.h
uiserver_p.h
)

ki18n_wrap_ui(kuiserver_KDEINIT_SRCS configdialog.ui)


#<DBus Stuff>

#<JobView>
  set(jobview_xml org.kde.JobView.xml)

  qt5_add_dbus_adaptor(kuiserver_KDEINIT_SRCS ${jobview_xml} jobview.h JobView jobviewadaptor )

  qt5_add_dbus_interface(kuiserver_KDEINIT_SRCS ${jobview_xml} jobview_interface )
#</JobView>


#<JobViewServer>
  set(jobviewserver_xml ${KJOBWIDGETS_DBUS_INTERFACES_DIR}/kf5_org.kde.JobViewServer.xml)

  qt5_add_dbus_adaptor(kuiserver_KDEINIT_SRCS ${jobviewserver_xml} progresslistmodel.h ProgressListModel jobviewserveradaptor )

  qt5_add_dbus_interface(kuiserver_KDEINIT_SRCS ${jobviewserver_xml} jobviewserver_interface )
#</JobViewServer>

qt5_add_dbus_adaptor(kuiserver_KDEINIT_SRCS org.kde.kuiserver.xml progresslistmodel.h ProgressListModel kuiserveradaptor )

configure_file(org.kde.kuiserver.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kuiserver.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kuiserver.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR} RENAME kf5_org.kde.kuiserver.service)

#</DBus Stuff>


kconfig_add_kcfg_files(kuiserver_KDEINIT_SRCS kuiserversettings.kcfgc)

kf5_add_kdeinit_executable( kuiserver5 ${kuiserver_KDEINIT_SRCS})

if (Q_WS_MAC)
    set_target_properties(kuiserver5 PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
    set_target_properties(kuiserver5 PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.kuiserver")
    set_target_properties(kuiserver5 PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE UI Server")
endif ()


target_link_libraries(kdeinit_kuiserver5
    Qt5::DBus
    KF5::ConfigWidgets
    KF5::DBusAddons
    KF5::ItemViews
    KF5::Notifications
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::XmlGui
    KF5::I18n
)
install(TARGETS kdeinit_kuiserver5 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( kuiserver5 kdeinit_kuiserver5 )
install(TARGETS kuiserver5 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kuiserver.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
