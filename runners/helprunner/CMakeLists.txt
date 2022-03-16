kcoreaddons_add_plugin(helprunner SOURCES helprunner.cpp helprunner.h INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(helprunner Qt::Widgets KF5::I18n KF5::Runner KF5::KIOGui)
