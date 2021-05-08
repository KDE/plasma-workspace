# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcmformats\")

set(kcm_formats_PART_SRCS
    kcmformats.cpp
)

ki18n_wrap_ui(kcm_formats_PART_SRCS kcmformatswidget.ui )

kcoreaddons_add_plugin(kcm_formats SOURCES ${kcm_formats_PART_SRCS} INSTALL_NAMESPACE "kcms")

kcoreaddons_desktop_to_json(kcm_formats formats.desktop SERVICE_TYPES kcmodule.desktop)

target_link_libraries(kcm_formats Qt::Core Qt::DBus Qt::Widgets KF5::I18n KF5::ConfigCore KF5::KCMUtils)

########### install files ###############

install( FILES formats.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
