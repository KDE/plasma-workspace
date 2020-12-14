add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_recentdocuments\")

set(krunner_recentdocuments_SRCS
    recentdocuments.cpp
)

add_library(krunner_recentdocuments MODULE ${krunner_recentdocuments_SRCS})
kcoreaddons_desktop_to_json(krunner_recentdocuments plasma-runner-recentdocuments.desktop)
target_link_libraries(krunner_recentdocuments
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::ActivitiesStats
    KF5::Runner
    KF5::Notifications
)

install(TARGETS krunner_recentdocuments DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )
