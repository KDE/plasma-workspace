set(keystate_engine_SRCS
    keystate.cpp
    keyservice.cpp
)

add_library(plasma_engine_keystate MODULE ${keystate_engine_SRCS})

target_link_libraries(plasma_engine_keystate
    KF5::GuiAddons
    KF5::Plasma
    KF5::Service
    KF5::KIOCore
    KF5::I18n
)

kservice_desktop_to_json(plasma_engine_keystate plasma-dataengine-keystate.desktop)

install(TARGETS plasma_engine_keystate DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-keystate.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install(FILES modifierkeystate.operations DESTINATION ${DATA_INSTALL_DIR}/plasma/services)

