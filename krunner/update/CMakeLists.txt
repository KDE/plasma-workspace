set(krunnerglobalshortcuts_SRCS
   krunnerglobalshortcuts.cpp
)

add_executable(krunnerglobalshortcuts ${krunnerglobalshortcuts_SRCS})

target_link_libraries(krunnerglobalshortcuts KF6::CoreAddons KF6::Service KF6::Runner KF6::ConfigCore KF6::GlobalAccel KF6::XmlGui)

add_executable(krunnerhistory krunnerhistory.cpp)
target_link_libraries(krunnerhistory KF6::Runner KF6::ConfigCore Plasma::Activities)

install(TARGETS krunnerglobalshortcuts krunnerhistory DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
install(FILES krunnerglobalshortcuts2.upd krunnerhistory.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
