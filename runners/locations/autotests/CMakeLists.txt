ecm_add_test(locationsrunnertest.cpp TEST_NAME locationsrunnertest LINK_LIBRARIES Qt::Test Qt::Gui KF5::Runner KF5::KIOCore)
krunner_configure_test(locationsrunnertest locations)
