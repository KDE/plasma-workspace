include(ECMAddTests)

ecm_add_tests(
    tasktoolstest.cpp
    launchertasksmodeltest.cpp
    LINK_LIBRARIES taskmanager Qt5::Test KF5::Service KF5::IconThemes
)
