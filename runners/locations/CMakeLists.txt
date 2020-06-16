add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_locations\")

set(krunner_locations_SRCS
    locationrunner.cpp
)

add_library(krunner_locations MODULE ${krunner_locations_SRCS})
kcoreaddons_desktop_to_json(krunner_locations plasma-runner-locations.desktop)
target_link_libraries(krunner_locations
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::Runner
)

install(TARGETS krunner_locations DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )
