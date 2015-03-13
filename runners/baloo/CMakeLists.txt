add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_baloosearchrunner\")

add_library(krunner_baloosearchrunner MODULE baloosearchrunner.cpp)

target_link_libraries(krunner_baloosearchrunner
  KF5::Runner
  KF5::KIOWidgets
  KF5::I18n
  KF5::Baloo
)

install(
  TARGETS krunner_baloosearchrunner
  DESTINATION ${KDE_INSTALL_PLUGINDIR}
)

install(
  FILES plasma-runner-baloosearch.desktop
  DESTINATION ${KDE_INSTALL_KSERVICES5DIR}
)
