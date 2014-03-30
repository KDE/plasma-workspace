########### kstartupconfig ###############

set(kstartupconfig_SRCS kstartupconfig.cpp )

add_executable(kstartupconfig5 ${kstartupconfig_SRCS})

install(TARGETS kstartupconfig5 ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### kdostartupconfig ###############

set(kdostartupconfig_SRCS kdostartupconfig.cpp )

add_executable(kdostartupconfig5 ${kdostartupconfig_SRCS})

target_link_libraries(kdostartupconfig5 Qt5::Core KF5::KDE4Support ${LIBBSD_LIBRARIES})

install(TARGETS kdostartupconfig5 ${INSTALL_TARGETS_DEFAULT_ARGS} )

