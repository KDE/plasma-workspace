set (ion_envcan_SRCS ion_envcan.cpp)
ecm_qt_declare_logging_category(ion_envcan_SRCS
    HEADER ion_envcandebug.h
    IDENTIFIER IONENGINE_ENVCAN
    CATEGORY_NAME kde.dataengine.ion.envcan
    DEFAULT_SEVERITY Info
)
add_library(ion_envcan MODULE ${ion_envcan_SRCS})
target_link_libraries (ion_envcan
    weather_ion
    KF5::KIOCore
    KF5::UnitConversion
    KF5::I18n
)

kcoreaddons_desktop_to_json(ion_envcan ion-envcan.desktop SERVICE_TYPES plasma-dataengine.desktop)

install (FILES ion-envcan.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install (TARGETS ion_envcan DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

