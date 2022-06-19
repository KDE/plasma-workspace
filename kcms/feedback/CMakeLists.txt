add_definitions(-DTRANSLATION_DOMAIN=\"kcm_feedback\")

set(kcm_feedback_PART_SRCS feedback.cpp)

kcmutils_generate_module_data(
    kcm_feedback_PART_SRCS
    MODULE_DATA_HEADER feedbackdata.h
    MODULE_DATA_CLASS_NAME FeedbackData
    SETTINGS_HEADERS feedbacksettings.h
    SETTINGS_CLASSES FeedbackSettings
)

kconfig_add_kcfg_files(kcm_feedback_PART_SRCS feedbacksettings.kcfgc GENERATE_MOC)
kcoreaddons_add_plugin(kcm_feedback SOURCES ${kcm_feedback_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file (kcm_feedback)

target_link_libraries(kcm_feedback
    KF5::I18n
    KF5::KCMUtils
    KF5::QuickAddons
    KUserFeedbackCore
)

ecm_qt_declare_logging_category(kcm_feedback
    HEADER kcm_feedback_debug.h
    IDENTIFIER KCM_FEEDBACK_DEBUG
    CATEGORY_NAME org.kde.plasma.kcm_feedback
)

install(FILES feedbacksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
kpackage_install_package(package kcm_feedback kcms)
