set (ion_envcan_SRCS ion_envcan.cpp)
add_library(ion_envcan MODULE ${ion_envcan_SRCS})
target_link_libraries (ion_envcan KF5::Solid weather_ion)

install (FILES ion-envcan.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install (TARGETS ion_envcan DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)

