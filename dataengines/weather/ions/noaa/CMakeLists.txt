set (ion_noaa_SRCS ion_noaa.cpp)
add_library(ion_noaa MODULE ${ion_noaa_SRCS})
target_link_libraries (ion_noaa KF5::Solid KF5::KDELibs4Support KF5::I18n weather_ion)

kservice_desktop_to_json(ion_noaa ion-noaa.desktop)

install (FILES ion-noaa.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install (TARGETS ion_noaa DESTINATION ${PLASMA_DATA_INSTALL_DIR}/dataengine)

