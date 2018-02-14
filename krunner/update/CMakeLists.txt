set(krunnerplugins_SRCS
   main.cpp
 )

add_executable(krunnerplugins ${krunnerplugins_SRCS})

target_link_libraries(krunnerplugins KF5::CoreAddons KF5::Service KF5::Runner KF5::ConfigCore)

install(TARGETS krunnerplugins DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
install(FILES krunnerplugins.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})


