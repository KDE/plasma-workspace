add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_appstream")

set(krunner_appstream_SRCS
    appstreamrunner.cpp
)

ecm_qt_declare_logging_category(krunner_appstream_SRCS
    HEADER debug.h
    IDENTIFIER RUNNER_APPSTREAM
    CATEGORY_NAME org.kde.plasma.runner.appstream
    DEFAULT_SEVERITY Warning)

add_library(krunner_appstream MODULE ${krunner_appstream_SRCS})
kcoreaddons_desktop_to_json(krunner_appstream plasma-runner-appstream.desktop )
target_link_libraries(krunner_appstream PUBLIC KF5::Runner KF5::I18n KF5::Service AppStreamQt)

install(TARGETS krunner_appstream DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")
