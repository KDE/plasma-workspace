include(ECMAddTests)

ecm_add_tests(
    launchertasksmodeltest.cpp
    tasktoolstest.cpp
    LINK_LIBRARIES taskmanager Qt5::Test KF5::Service KF5::IconThemes
)
