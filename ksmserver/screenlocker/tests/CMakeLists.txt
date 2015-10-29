add_executable(kscreenlocker_test kscreenlocker_main.cpp)
target_link_libraries(kscreenlocker_test ksld Qt5::Widgets KF5::I18n)
ecm_mark_as_test(kscreenlocker_test)
