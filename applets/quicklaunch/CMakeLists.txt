project(plasma-quicklaunch)

set(quicklaunch_SRCS
  icongridlayout.cpp
  launcher.cpp
  launcherdata.cpp
  launchergrid.cpp
  popup.cpp
  popuplauncherlist.cpp
  quicklaunch.cpp
)

kde4_add_ui_files(quicklaunch_SRCS quicklaunchConfig.ui)
add_library(plasma_applet_quicklaunch MODULE ${quicklaunch_SRCS})

target_link_libraries(plasma_applet_quicklaunch
  KF5::Plasma
  KIOCore
)

install(TARGETS plasma_applet_quicklaunch DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-quicklaunch.desktop DESTINATION ${SERVICES_INSTALL_DIR})
