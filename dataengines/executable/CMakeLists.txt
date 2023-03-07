set(executable_engine_SRCS
    executable.cpp
    executable.h
)

kcoreaddons_add_plugin(plasma_engine_executable SOURCES ${executable_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_executable
    KF6::Plasma
    KF6::Plasma5Support
    KF6::Service
    KF6::CoreAddons
)
