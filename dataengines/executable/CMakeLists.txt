add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_executable\")

set(executable_engine_SRCS
    executable.cpp
    executable.h
)

kcoreaddons_add_plugin(plasma_engine_executable SOURCES ${executable_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_executable
    Plasma::Plasma
    Plasma::Plasma5Support
    KF6::Service
    KF6::CoreAddons
    KF6::I18n
    KF6::Notifications
)

install(FILES plasma_dataengine_executable.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
