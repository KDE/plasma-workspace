add_definitions(-DTRANSLATION_DOMAIN=\"kcm_feedback\")

add_library(kcm_feedback MODULE feedback.cpp)

target_link_libraries(kcm_feedback
    KF5::I18n
    KF5::KCMUtils
    KF5::QuickAddons
    KUserFeedbackCore
)

kcoreaddons_desktop_to_json(kcm_feedback "kcm_feedback.desktop")

install(TARGETS kcm_feedback DESTINATION ${KDE_INSTALL_PLUGINDIR}/kcms)
install(FILES kcm_feedback.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})
kpackage_install_package(package kcm_feedback kcms)
