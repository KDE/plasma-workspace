set(kde_PART_SRCS
	kdeplatformplugin.cpp
	kiomediastream.cpp
	devicelisting.cpp
)

if(ALSA_FOUND)
   qt5_add_resources(kde_PART_SRCS resources.qrc)
endif()

add_library(kde MODULE ${kde_PART_SRCS})
target_link_libraries(kde ${PHONON_LIBRARY} KF5::KDE4Support KF5::KIOWidgets)

kservice_desktop_to_json(kde phononbackend.desktop)

if(ALSA_FOUND)
   target_link_libraries(kde ${ASOUND_LIBRARY})
endif()

if(NOT WIN32)
   set_target_properties(kde PROPERTIES
                             INSTALL_RPATH_USE_LINK_PATH TRUE
                             SKIP_BUILD_RPATH TRUE
                             BUILD_WITH_INSTALL_RPATH TRUE
                             INSTALL_RPATH ${LIB_INSTALL_DIR}
   )
endif()

install(TARGETS kde DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/phonon_platform)
install(FILES phonon.notifyrc DESTINATION ${DATA_INSTALL_DIR}/phonon)
install(FILES phononbackend.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
