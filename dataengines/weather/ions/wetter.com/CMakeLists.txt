set(ion_wettercom_SRCS ion_wettercom.cpp)
add_library(ion_wettercom MODULE ${ion_wettercom_SRCS})
target_link_libraries(ion_wettercom weather_ion KF5::KDELibs4Support KF5::CoreAddons KF5::Plasma KF5::KIOCore ${KDE4_KUNITCONVERSION_LIBS})

kcoreaddons_desktop_to_json(ion_wettercom ion-wettercom.desktop)

install(FILES ion-wettercom.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(TARGETS ion_wettercom DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

