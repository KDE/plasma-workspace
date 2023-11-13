add_definitions(-DTRANSLATION_DOMAIN=\"kcm_feedback\")

kcmutils_add_qml_kcm(kcm_feedback SOURCES feedback.cpp)

kcmutils_generate_module_data(kcm_feedback
    MODULE_DATA_HEADER feedbackdata.h
    MODULE_DATA_CLASS_NAME FeedbackData
    SETTINGS_HEADERS feedbacksettings.h
    SETTINGS_CLASSES FeedbackSettings
)
kconfig_add_kcfg_files(kcm_feedback feedbacksettings.kcfgc GENERATE_MOC)
target_link_libraries(kcm_feedback PRIVATE
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
    KF6::UserFeedbackCore
)

ecm_qt_declare_logging_category(kcm_feedback
    HEADER kcm_feedback_debug.h
    IDENTIFIER KCM_FEEDBACK_DEBUG
    CATEGORY_NAME org.kde.plasma.kcm_feedback
    DESCRIPTION "kcm feedback"
    EXPORT PLASMAWORKSPACE
)

install(FILES feedbacksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
