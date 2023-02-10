add_executable(kcminit main.cpp)
target_link_libraries(kcminit
    Qt::Core
    Qt::Gui
    Qt::DBus
    KF6::CoreAddons
    KF6::Service
    KF6::I18n
    KF6::ConfigCore
    PW::KWorkspace
)
install(TARGETS kcminit ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_custom_command(TARGET kcminit POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink kcminit kcminit_startup)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcminit_startup DESTINATION ${KDE_INSTALL_BINDIR})

ecm_install_configured_files(
    INPUT
        plasma-kcminit-phase1.service.in
        plasma-kcminit.service.in
    DESTINATION
        ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
