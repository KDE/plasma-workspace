add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_locations\")

set(krunner_locations_SRCS
    locationrunner.cpp
)

kcoreaddons_add_plugin(locations SOURCES ${krunner_locations_SRCS} INSTALL_NAMESPACE "kf5/krunner")
kcoreaddons_desktop_to_json(locations plasma-runner-locations.desktop)
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
