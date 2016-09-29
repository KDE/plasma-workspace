set(krunnerplugins_SRCS
   main.cpp
 )

add_executable(krunnerplugins ${krunnerplugins_SRCS})

target_link_libraries(krunnerplugins KF5::CoreAddons KF5::Service KF5::Runner KF5::ConfigCore)

install(TARGETS krunnerplugins DESTINATION ${LIB_INSTALL_DIR}/kconf_update_bin/)
install(FILES krunnerplugins.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR})


