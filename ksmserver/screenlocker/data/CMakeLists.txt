set( force_krunner_lock_shortcut_unreg_SRCS force_krunner_lock_shortcut_unreg.cpp )

add_executable( force_krunner_lock_shortcut_unreg ${force_krunner_lock_shortcut_unreg_SRCS} )

target_link_libraries( force_krunner_lock_shortcut_unreg Qt5::Core Qt5::DBus )

install( TARGETS force_krunner_lock_shortcut_unreg DESTINATION ${LIB_INSTALL_DIR}/kconf_update_bin/ )
install( FILES kscreenlocker_locksession-shortcut.upd DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
