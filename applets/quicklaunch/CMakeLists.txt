add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_quicklaunch\")

set(quicklaunch_SRCS
  icongridlayout.cpp
  launcher.cpp
  launcherdata.cpp
  launchergrid.cpp
  popup.cpp
  popuplauncherlist.cpp
  quicklaunch.cpp
)

ki18n_wrap_ui(quicklaunch_SRCS quicklaunchConfig.ui)
add_library(plasma_applet_quicklaunch MODULE ${quicklaunch_SRCS})

target_link_libraries(plasma_applet_quicklaunch
  KF5::Plasma
  KIOCore
)

install(TARGETS plasma_applet_quicklaunch DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-applet-quicklaunch.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
