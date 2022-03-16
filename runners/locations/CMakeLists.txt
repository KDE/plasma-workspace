add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_locations\")

set(krunner_locations_SRCS
    locationrunner.cpp
    locationrunner.h
)

kcoreaddons_add_plugin(locations SOURCES ${krunner_locations_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(locations
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::Runner
    KF5::Notifications
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
