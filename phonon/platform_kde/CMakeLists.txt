set(kde_PART_SRCS
    debug.cpp
    kdeplatformplugin.cpp
    kiomediastream.cpp
)

kcoreaddons_add_plugin(kde SOURCES ${kde_PART_SRCS} INSTALL_NAMESPACE "phonon_platform")
target_link_libraries(kde ${PHONON_LIBRARY} KF5::CoreAddons KF5::ConfigCore KF5::WidgetsAddons KF5::I18n KF5::KIOCore KF5::Notifications)

kcoreaddons_desktop_to_json(kde phononbackend.desktop)

install(FILES phonon.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})
install(FILES phononbackend.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
