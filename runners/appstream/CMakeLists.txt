add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_appstream")

set(krunner_appstream_SRCS
    appstreamrunner.cpp
)

ecm_qt_declare_logging_category(krunner_appstream_SRCS
    HEADER debug.h
    IDENTIFIER RUNNER_APPSTREAM
    CATEGORY_NAME org.kde.plasma.runner.appstream
    DEFAULT_SEVERITY Warning)

kcoreaddons_add_plugin(krunner_appstream SOURCES ${krunner_appstream_SRCS} INSTALL_NAMESPACE "kf5/krunner")
kcoreaddons_desktop_to_json(krunner_appstream plasma-runner-appstream.desktop )
target_link_libraries(krunner_appstream PUBLIC KF5::Runner KF5::I18n KF5::Service AppStreamQt)
