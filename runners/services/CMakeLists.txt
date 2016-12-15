add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_services\")

set(krunner_services_SRCS
    servicerunner.cpp
)

ecm_qt_declare_logging_category(krunner_services_SRCS
    HEADER debug.h
    IDENTIFIER RUNNER_SERVICES
    CATEGORY_NAME org.kde.plasma.runner.services
    DEFAULT_SEVERITY Warning)

add_library(krunner_services MODULE ${krunner_services_SRCS})
target_link_libraries(krunner_services
    KF5::KIOWidgets
    KF5::I18n
    KF5::Runner
    KF5::Service
    KF5::Activities
)

install(TARGETS krunner_services DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES plasma-runner-services.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
