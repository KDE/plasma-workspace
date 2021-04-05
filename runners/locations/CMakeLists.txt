add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_locations\")

set(krunner_locations_SRCS
    locationrunner.cpp
)

add_library(locations MODULE ${krunner_locations_SRCS})
kcoreaddons_desktop_to_json(locations plasma-runner-locations.desktop)
target_link_libraries(locations
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::Runner
    KF5::Notifications
)

set_target_properties(locations PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/krunner")
install(TARGETS locations DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
