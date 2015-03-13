add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_recentdocuments\")

set(krunner_recentdocuments_SRCS
    recentdocuments.cpp
)

add_library(krunner_recentdocuments MODULE ${krunner_recentdocuments_SRCS})

target_link_libraries(krunner_recentdocuments
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::Runner
)

install(TARGETS krunner_recentdocuments DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES recentdocuments.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
