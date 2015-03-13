set(executable_engine_SRCS
    executable.cpp
)

add_library(plasma_engine_executable MODULE ${executable_engine_SRCS})

target_link_libraries(plasma_engine_executable
    KF5::Plasma
    KF5::Service
    KF5::CoreAddons
)

kcoreaddons_desktop_to_json(plasma_engine_executable plasma-dataengine-executable.desktop)

install(TARGETS plasma_engine_executable DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-executable.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
