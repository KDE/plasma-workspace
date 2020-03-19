remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(ECMAddTests)

ecm_add_test(servicerunnertest.cpp TEST_NAME servicerunnertest
    LINK_LIBRARIES Qt5::Test krunner_services_static)
