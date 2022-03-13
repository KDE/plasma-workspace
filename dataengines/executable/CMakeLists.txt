set(executable_engine_SRCS
    executable.cpp
    executable.h
)

kcoreaddons_add_plugin(plasma_engine_executable SOURCES ${executable_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)

target_link_libraries(plasma_engine_executable
    KF5::Plasma
    KF5::Service
    KF5::CoreAddons
)
