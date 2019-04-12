include(ECMMarkAsTest)

set(notifications_test_SRCS
    notifications_test.cpp
)
add_executable(notification_test  ${notifications_test_SRCS})
target_link_libraries(notification_test Qt5::Test Qt5::Core PW::LibNotificationManager)
ecm_mark_as_test(notification_test)
