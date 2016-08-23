set (ion_bbcukmet_SRCS ion_bbcukmet.cpp)
ecm_qt_declare_logging_category(ion_bbcukmet_SRCS
    HEADER ion_bbcukmetdebug.h
    IDENTIFIER IONENGINE_BBCUKMET
    CATEGORY_NAME kde.dataengine.ion.bbcukmet
    DEFAULT_SEVERITY Info
)
add_library(ion_bbcukmet MODULE ${ion_bbcukmet_SRCS})
target_link_libraries (ion_bbcukmet
    weather_ion
    KF5::KIOCore
    KF5::UnitConversion
    KF5::I18n
)

kcoreaddons_desktop_to_json(ion_bbcukmet ion-bbcukmet.desktop SERVICE_TYPES plasma-dataengine.desktop)

install (FILES ion-bbcukmet.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install (TARGETS ion_bbcukmet DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

