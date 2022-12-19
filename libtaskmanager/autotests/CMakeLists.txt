include(ECMAddTests)

ecm_add_tests(
    tasktoolstest.cpp
    tasksmodeltest.cpp
    launchertasksmodeltest.cpp
    xwindowtasksmodeltest.cpp
    LINK_LIBRARIES taskmanager Qt::Test KF5::Service KF5::IconThemes KF5::ConfigCore KF5::WindowSystem
)
