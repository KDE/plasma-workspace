set(ion_wettercom_SRCS ion_wettercom.cpp)
add_library(ion_wettercom MODULE ${ion_wettercom_SRCS})
target_link_libraries(ion_wettercom
    weather_ion
    KF5::KIOCore
    KF5::UnitConversion
    KF5::I18n
)

kcoreaddons_desktop_to_json(ion_wettercom ion-wettercom.desktop)

install(FILES ion-wettercom.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(TARGETS ion_wettercom DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

