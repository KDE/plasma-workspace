set(kde_PART_SRCS
    debug.cpp
    kdeplatformplugin.cpp
    kiomediastream.cpp
)

add_library(kde MODULE ${kde_PART_SRCS})
target_link_libraries(kde ${PHONON_LIBRARY} KF5::I18n KF5::KIOWidgets KF5::Notifications)

kservice_desktop_to_json(kde phononbackend.desktop)

if(NOT WIN32)
   set_target_properties(kde PROPERTIES
                             INSTALL_RPATH_USE_LINK_PATH TRUE
                             SKIP_BUILD_RPATH TRUE
                             BUILD_WITH_INSTALL_RPATH TRUE
                             INSTALL_RPATH ${LIB_INSTALL_DIR}
   )
endif()

install(TARGETS kde DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/phonon_platform)
install(FILES phonon.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
install(FILES phononbackend.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
