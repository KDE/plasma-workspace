set(krunnerglobalshortcuts_SRCS
   krunnerglobalshortcuts.cpp
 )

add_executable(krunnerglobalshortcuts ${krunnerglobalshortcuts_SRCS})

target_link_libraries(krunnerglobalshortcuts KF5::CoreAddons KF5::Service KF5::Runner KF5::ConfigCore KF5::GlobalAccel KF5::XmlGui)

add_executable(krunnerhistory krunnerhistory.cpp)
target_link_libraries(krunnerhistory KF5::Runner KF5::ConfigCore KF5::Activities)

install(TARGETS krunnerglobalshortcuts krunnerhistory DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
install(FILES krunnerglobalshortcuts.upd krunnerhistory.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
