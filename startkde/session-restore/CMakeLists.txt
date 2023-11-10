add_executable(plasma-fallback-session-save
    save.cpp
)
target_link_libraries(plasma-fallback-session-save
    Qt6::Core
    KF6::KIOGui
    KF6::ConfigCore
    KF6::WaylandClient
)

add_executable( plasma-fallback-session-restore
    restore.cpp
)
ecm_qt_declare_logging_category(plasma-fallback-session-restore HEADER debug.h IDENTIFIER FALLBACK_SESSION_RESTORE CATEGORY_NAME org.kde.plasma-fallback-session-restore)

target_link_libraries( plasma-fallback-session-restore
    Qt6::Core
    KF6::KIOGui
    KF6::ConfigCore
)

install(TARGETS plasma-fallback-session-restore DESTINATION ${KDE_INSTALL_FULL_LIBEXECDIR})
install(TARGETS plasma-fallback-session-save DESTINATION ${KDE_INSTALL_FULL_LIBEXECDIR})

ecm_install_configured_files(INPUT org.kde.plasma-fallback-session-save.desktop.in @ONLY DESTINATION ${KDE_INSTALL_APPDIR})
ecm_install_configured_files(INPUT org.kde.plasma-fallback-session-restore.desktop.in @ONLY DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
