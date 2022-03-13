add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_keystate\")

set(keystate_engine_SRCS
    keystate.cpp
    keyservice.cpp
    keystate.h
    keyservice.h
)

kcoreaddons_add_plugin(plasma_engine_keystate SOURCES ${keystate_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_keystate
    KF5::GuiAddons
    KF5::Plasma
    KF5::Service
    KF5::KIOCore
    KF5::I18n
)

install(FILES modifierkeystate.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
