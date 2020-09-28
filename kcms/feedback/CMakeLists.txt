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
add_library(kcm_feedback MODULE ${kcm_feedback_PART_SRCS})

target_link_libraries(kcm_feedback
    KF5::I18n
    KF5::KCMUtils
    KF5::QuickAddons
    KUserFeedbackCore
)

kcoreaddons_desktop_to_json(kcm_feedback "kcm_feedback.desktop")

install(FILES feedbacksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(TARGETS kcm_feedback DESTINATION ${KDE_INSTALL_PLUGINDIR}/kcms)
install(FILES kcm_feedback.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})
kpackage_install_package(package kcm_feedback kcms)
