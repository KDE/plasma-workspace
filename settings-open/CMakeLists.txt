add_executable(settings-open main.cpp)
target_link_libraries(settings-open Qt::Core KF5::CoreAddons)
install(TARGETS settings-open ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.plasma.settings.open.desktop DESTINATION  ${KDE_INSTALL_APPDIR})
