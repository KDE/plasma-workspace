
# TODO: merge into kstartupconfig5
add_executable(kdostartupconfig5 kdostartupconfig.cpp)

target_link_libraries(kdostartupconfig5
                      Qt5::Core
                      KF5::ConfigCore
                      KF5::CoreAddons
                      ${LIBBSD_LIBRARIES})

install(TARGETS kdostartupconfig5 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

