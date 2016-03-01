set (ion_envcan_SRCS ion_envcan.cpp)
add_library(ion_envcan MODULE ${ion_envcan_SRCS})
target_link_libraries (ion_envcan
    weather_ion
    KF5::KIOCore
    KF5::UnitConversion
    KF5::I18n
)

kcoreaddons_desktop_to_json(ion_envcan ion-envcan.desktop)

install (FILES ion-envcan.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install (TARGETS ion_envcan DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

