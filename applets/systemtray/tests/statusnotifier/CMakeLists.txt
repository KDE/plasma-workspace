set(statusnotifiertest_SRCS
    main.cpp
    statusnotifiertest.cpp
    pumpjob.cpp
)

ki18n_wrap_ui(statusnotifiertest_SRCS statusnotifiertest.ui)

add_executable(statusnotifiertest ${statusnotifiertest_SRCS})

target_link_libraries(statusnotifiertest
    Qt::Widgets
    Qt::Core
    KF6::CoreAddons
    KF6::KIOCore
    KF6::Service
    KF6::Notifications
    KF6::I18n
    KF6::ConfigCore
    Qt::DBus
)

include(ECMMarkAsTest)
ecm_mark_as_test(statusnotifiertest)
