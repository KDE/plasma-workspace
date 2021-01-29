add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_dict\")

set(dict_engine_SRCS
    dictengine.cpp
)

add_library(plasma_engine_dict MODULE ${dict_engine_SRCS})

target_link_libraries (plasma_engine_dict
  KF5::Plasma
  KF5::Service
  KF5::I18n
  Qt::Network
)

kcoreaddons_desktop_to_json(plasma_engine_dict plasma-dataengine-dict.desktop)

install(TARGETS plasma_engine_dict DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-dict.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
