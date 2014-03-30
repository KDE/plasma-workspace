set(ion_wettercom_SRCS ion_wettercom.cpp)
add_library(ion_wettercom MODULE ${ion_wettercom_SRCS})
target_link_libraries(ion_wettercom weather_ion KF5::CoreAddons KF5::Plasma KIOCore ${KDE4_KUNITCONVERSION_LIBS})

install(FILES ion-wettercom.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS ion_wettercom DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)

