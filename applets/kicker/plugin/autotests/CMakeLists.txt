ecm_add_test(
    testrunnermodel.cpp
    LINK_LIBRARIES kickerplugin Qt::Test
)
set_tests_properties(testrunnermodel PROPERTIES REPEAT UNTIL_PASS:2)
