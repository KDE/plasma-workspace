kcoreaddons_add_plugin(helprunner SOURCES helprunner.cpp INSTALL_NAMESPACE "kf5/krunner")
target_link_libraries(helprunner Qt::Widgets KF5::I18n KF5::Runner)
