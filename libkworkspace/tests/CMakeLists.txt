add_executable(sessiontest sessiontest.cpp)
target_link_libraries(sessiontest PW::KWorkspace)

add_executable(sessionmodeltest sessionmodeltest)
target_link_libraries(sessionmodeltest PW::KWorkspace Qt::DBus)
