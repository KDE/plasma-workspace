ecm_add_test(
    testrunnermodel.cpp
    ../runnermodel.cpp
    ../runnermatchesmodel.cpp
    ../abstractmodel.cpp
    ../actionlist.cpp
    ../containmentinterface.cpp
    ../processrunner.cpp
    TEST_NAME testrunnermodel
    LINK_LIBRARIES Qt::Test Qt::Quick KF6::ConfigCore KF6::Runner KF6::Service KF6::I18n KF6::KIOGui KF6::KIOWidgets KF6::JobWidgets Plasma::Plasma Plasma::PlasmaQuick Plasma::Activities Plasma::ActivitiesStats
)

if (${HAVE_APPSTREAMQT})
target_link_libraries(testrunnermodel AppStreamQt)
endif()

set_tests_properties(testrunnermodel PROPERTIES RUN_SERIAL ON)
