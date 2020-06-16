add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_services\")

set(krunner_services_SRCS
    servicerunner.cpp
)

ecm_qt_declare_logging_category(krunner_services_SRCS
    HEADER debug.h
    IDENTIFIER RUNNER_SERVICES
    CATEGORY_NAME org.kde.plasma.runner.services
    DEFAULT_SEVERITY Warning)

add_library(krunner_services_static STATIC ${krunner_services_SRCS})
target_link_libraries(krunner_services_static
    KF5::CoreAddons
    KF5::KIOGui
    KF5::I18n
    KF5::Notifications
    KF5::Runner
    KF5::Service
    KF5::Activities
)

add_library(krunner_services MODULE plugin.cpp)
kcoreaddons_desktop_to_json(krunner_services plasma-runner-services.desktop)
target_link_libraries(krunner_services
    krunner_services_static
)

install(TARGETS krunner_services DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()
