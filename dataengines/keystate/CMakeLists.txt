add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_keystate\")

set(keystate_engine_SRCS
    keystate.cpp
    keyservice.cpp
    keystate.h
    keyservice.h
)

kcoreaddons_add_plugin(plasma_engine_keystate SOURCES ${keystate_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_keystate
    KF6::GuiAddons
    Plasma::Plasma
    KF6::Plasma5Support
    KF6::Service
    KF6::KIOCore
    KF6::I18n
)

install(FILES modifierkeystate.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
