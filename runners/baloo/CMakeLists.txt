add_library(krunner_baloosearchrunner MODULE baloosearchrunner.cpp)

target_link_libraries(krunner_baloosearchrunner
  KF5::Runner
  KF5::KIOWidgets
  KF5::I18n
  KF5::BalooCore
)

install(
  TARGETS krunner_baloosearchrunner
  DESTINATION ${PLUGIN_INSTALL_DIR}
)

install(
  FILES plasma-runner-baloosearch.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
)
