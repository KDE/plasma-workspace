set(kde_PART_SRCS
    debug.cpp
    kdeplatformplugin.cpp
    kiomediastream.cpp
)

add_library(kde MODULE ${kde_PART_SRCS})
target_link_libraries(kde ${PHONON_LIBRARY} KF5::I18n KF5::KIOWidgets KF5::Notifications)

kcoreaddons_desktop_to_json(kde phononbackend.desktop)

if(NOT WIN32)
   set_target_properties(kde PROPERTIES
                             INSTALL_RPATH_USE_LINK_PATH TRUE
                             SKIP_BUILD_RPATH TRUE
                             BUILD_WITH_INSTALL_RPATH TRUE
                             INSTALL_RPATH ${KDE_INSTALL_LIBDIR}
   )
endif()

install(TARGETS kde DESTINATION ${KDE_INSTALL_PLUGINDIR}/phonon_platform)
install(FILES phonon.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})
install(FILES phononbackend.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
