add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_activities\")

set(krunner_activities_SRCS
    activityrunner.cpp
)

add_library(krunner_activities MODULE ${krunner_activities_SRCS})
target_link_libraries(krunner_activities KF5::Runner KF5::I18n KF5::Activities)

install(TARGETS krunner_activities DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-runner-activityrunner.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
