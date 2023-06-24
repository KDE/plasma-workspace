set (ion_noaa_SRCS ion_noaa.cpp)
ecm_qt_declare_logging_category(ion_noaa_SRCS
    HEADER ion_noaadebug.h
    IDENTIFIER IONENGINE_NOAA
    CATEGORY_NAME kde.dataengine.ion.noaa
    DEFAULT_SEVERITY Info
    DESCRIPTION "DataEngine ion noaa"
    EXPORT PLASMAWORKSPACE
)
add_library(plasma_engine_noaa MODULE ${ion_noaa_SRCS})
target_link_libraries (plasma_engine_noaa
    weather_ion
    KF6::KIOCore
    KF6::UnitConversion
    KF6::I18n
)

install (TARGETS plasma_engine_noaa DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)

