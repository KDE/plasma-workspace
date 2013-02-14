project(desktop_homescreen)

install(DIRECTORY . DESTINATION ${DATA_INSTALL_DIR}/plasma/packages/org.kde.homescreen.desktop PATTERN .svn EXCLUDE PATTERN CMakeLists.txt EXCLUDE PATTERN Messages.sh EXCLUDE)


