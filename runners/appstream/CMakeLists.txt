add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_appstream")

set(krunner_appstream_SRCS
    appstreamrunner.cpp
)

ecm_qt_declare_logging_category(krunner_appstream_SRCS
    HEADER debug.h
    IDENTIFIER RUNNER_APPSTREAM
    CATEGORY_NAME org.kde.plasma.runner.appstream
    DEFAULT_SEVERITY Warning)

kcoreaddons_add_plugin(krunner_appstream SOURCES ${krunner_appstream_SRCS} INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_appstream PUBLIC Qt::Gui KF6::Runner KF6::I18n KF6::Service AppStreamQt)
