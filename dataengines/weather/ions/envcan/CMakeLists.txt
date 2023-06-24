set (ion_envcan_SRCS ion_envcan.cpp)
ecm_qt_declare_logging_category(ion_envcan_SRCS
    HEADER ion_envcandebug.h
    IDENTIFIER IONENGINE_ENVCAN
    CATEGORY_NAME kde.dataengine.ion.envcan
    DEFAULT_SEVERITY Info
    DESCRIPTION "DataEngine ion envcan"
    EXPORT PLASMAWORKSPACE
)
add_library(plasma_engine_envcan MODULE ${ion_envcan_SRCS})
target_link_libraries (plasma_engine_envcan
    weather_ion
    KF6::KIOCore
    KF6::UnitConversion
    KF6::I18n
)

install (TARGETS plasma_engine_envcan DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)

