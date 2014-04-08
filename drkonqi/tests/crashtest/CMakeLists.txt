add_executable(crashtest crashtest.cpp)
ecm_mark_as_test(crashtest)
add_test(crashtest crashtest)

target_link_libraries(crashtest
    KF5::I18n KF5::Crash KF5::CoreAddons Qt5::Widgets
    Qt5::Concurrent)

