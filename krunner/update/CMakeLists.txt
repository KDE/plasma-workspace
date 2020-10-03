set(krunnerglobalshortcuts_SRCS
   krunnerglobalshortcuts.cpp
 )

add_executable(krunnerglobalshortcuts ${krunnerglobalshortcuts_SRCS})

target_link_libraries(krunnerglobalshortcuts KF5::CoreAddons KF5::Service KF5::Runner KF5::ConfigCore KF5::GlobalAccel KF5::XmlGui)

install(TARGETS krunnerglobalshortcuts DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
install(FILES krunnerglobalshortcuts.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
