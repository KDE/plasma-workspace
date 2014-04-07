set(kded_phonon_SRCS
   phononserver.cpp
   deviceinfo.cpp
   deviceaccess.cpp
   hardwaredatabase.cpp
)

add_library(kded_phononserver MODULE ${kded_phonon_SRCS})

target_link_libraries(kded_phononserver ${PHONON_LIBRARIES} KF5::ConfigCore KF5::DBusAddons KF5::Solid KF5::Service KF5::KDE4Support)

if(ALSA_FOUND)
   target_link_libraries(kded_phononserver ${ASOUND_LIBRARY})
endif()

install(TARGETS kded_phononserver  DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES phononserver.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES hardwaredatabase DESTINATION ${DATA_INSTALL_DIR}/libphonon)
