set (ion_noaa_SRCS ion_noaa.cpp)
ecm_qt_declare_logging_category(ion_noaa_SRCS
    HEADER ion_noaadebug.h
    IDENTIFIER IONENGINE_NOAA
    CATEGORY_NAME kde.dataengine.ion.noaa
    DEFAULT_SEVERITY Info
)
add_library(ion_noaa MODULE ${ion_noaa_SRCS})
target_link_libraries (ion_noaa
    weather_ion
    KF5::KIOCore
    KF5::UnitConversion
    KF5::I18n
)

install (TARGETS ion_noaa DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

