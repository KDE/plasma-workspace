set (ion_noaa_SRCS ion_noaa.cpp)
add_library(ion_noaa MODULE ${ion_noaa_SRCS})
target_link_libraries (ion_noaa KF5::Solid weather_ion)

install (FILES ion-noaa.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install (TARGETS ion_noaa DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)

