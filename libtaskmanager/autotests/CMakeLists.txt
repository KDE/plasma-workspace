include(ECMAddTests)

ecm_add_tests(
    tasktoolstest.cpp
    launchertasksmodeltest.cpp
    LINK_LIBRARIES taskmanager Qt::Test KF5::Service KF5::IconThemes
)
