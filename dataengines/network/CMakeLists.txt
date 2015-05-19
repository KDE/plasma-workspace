add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_network\")

set(network_engine_SRCS
    networkengine.cpp
)

add_library(plasma_engine_network MODULE ${network_engine_SRCS})
target_link_libraries(plasma_engine_network KF5::CoreAddons KF5::Plasma solidcontrol)

install(TARGETS plasma_engine_network DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-network.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

