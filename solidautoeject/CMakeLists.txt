########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"solidautoeject\")

set(kded_solidautoeject_SRCS
   solidautoeject.cpp
)

add_library(solidautoeject MODULE ${kded_solidautoeject_SRCS})
kcoreaddons_desktop_to_json(solidautoeject solidautoeject.desktop)

target_link_libraries(solidautoeject KF5::Solid KF5::DBusAddons KF5::CoreAddons)

install(TARGETS solidautoeject DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
