# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_lookandfeel\")

set(kcm_lookandfeel_SRCS
  kcm.cpp
  ../../lookandfeelaccess/lookandfeelaccess.cpp
)

add_library(kcm_lookandfeel MODULE ${kcm_lookandfeel_SRCS})

target_link_libraries(kcm_lookandfeel
  KF5::KIOWidgets
  KF5::CoreAddons
  KF5::KCMUtils
  KF5::I18n
  KF5::Plasma
  KF5::PlasmaQuick

  Qt5::DBus
  Qt5::Widgets
  Qt5::QuickWidgets
)


install(FILES kcm_lookandfeel.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS kcm_lookandfeel DESTINATION ${PLUGIN_INSTALL_DIR})

plasma_install_package(package kcm_lookandfeel kcms kcm_lookandfeel)
