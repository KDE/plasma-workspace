add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_ion_%{APPNAMELC}\")

add_library(ion_%{APPNAMELC} MODULE ion-%{APPNAMELC}.cpp)

target_link_libraries (ion_%{APPNAMELC}
    ${PlasmaWeatherIon_LIBRARIES}
    KF5::Plasma
    KF5::UnitConversion
)

kcoreaddons_desktop_to_json(ion_%{APPNAMELC} ion-%{APPNAMELC}.desktop SERVICE_TYPES plasma-dataengine.desktop)

install (FILES ion-%{APPNAMELC}.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install (TARGETS ion_%{APPNAMELC} DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
