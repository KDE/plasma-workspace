set(network_engine_SRCS
    networkengine.cpp
)

add_library(plasma_engine_network MODULE ${network_engine_SRCS})
target_link_libraries(plasma_engine_network KF5::CoreAddons KF5::Plasma solidcontrol)

install(TARGETS plasma_engine_network DESTINATION ${PLASMA_DATA_INSTALL_DIR}/dataengine)
install(FILES plasma-dataengine-network.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

