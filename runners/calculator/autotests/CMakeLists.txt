ecm_add_test(calculatorrunnertest.cpp TEST_NAME calculatorrunnertest LINK_LIBRARIES Qt::Test KF5::Runner KF5::KIOCore)
configure_krunner_test(calculatorrunnertest calculator)
