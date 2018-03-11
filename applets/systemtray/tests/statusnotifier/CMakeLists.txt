set(statusnotifiertest_SRCS
    main.cpp
    statusnotifiertest.cpp
    pumpjob.cpp
)

ki18n_wrap_ui(statusnotifiertest_SRCS statusnotifiertest.ui)

add_executable(statusnotifiertest ${statusnotifiertest_SRCS})

target_link_libraries(statusnotifiertest
    Qt5::Widgets
    Qt5::Core
    KF5::CoreAddons
    KF5::KIOCore
    KF5::Service
    KF5::Notifications
    KF5::I18n
    Qt5::DBus
)

include(ECMMarkAsTest)
ecm_mark_as_test(statusnotifiertest)
