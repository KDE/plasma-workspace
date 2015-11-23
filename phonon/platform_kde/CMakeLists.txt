set(kde_PART_SRCS
    debug.cpp
    kdeplatformplugin.cpp
    kiomediastream.cpp
)

add_library(kde MODULE ${kde_PART_SRCS})
target_link_libraries(kde ${PHONON_LIBRARY} KF5::I18n KF5::KIOWidgets KF5::Notifications)

kcoreaddons_desktop_to_json(kde phononbackend.desktop)

install(TARGETS kde DESTINATION ${KDE_INSTALL_PLUGINDIR}/phonon_platform)
install(FILES phonon.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})
install(FILES phononbackend.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
