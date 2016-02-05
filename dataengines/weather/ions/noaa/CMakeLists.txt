set (ion_noaa_SRCS ion_noaa.cpp)
add_library(ion_noaa MODULE ${ion_noaa_SRCS})
target_link_libraries (ion_noaa KF5::KDELibs4Support KF5::I18n weather_ion)

kcoreaddons_desktop_to_json(ion_noaa ion-noaa.desktop)

install (FILES ion-noaa.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install (TARGETS ion_noaa DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

