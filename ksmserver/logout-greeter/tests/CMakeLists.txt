add_executable(logout-greeter-test main.cpp)
target_link_libraries(logout-greeter-test Qt5::Core Qt5::Concurrent)
ecm_mark_as_test(logout-greeter-test)
