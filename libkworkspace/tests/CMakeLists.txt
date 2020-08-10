add_executable(sessiontest sessiontest.cpp)
target_link_libraries(sessiontest PW::KWorkspace)

add_executable(syncdbusenv syncdbusenvtest.cpp)
target_link_libraries(syncdbusenv PW::KWorkspace)
