########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"solidautoeject\")

set(kded_solidautoeject_SRCS
   solidautoeject.cpp
)

kcoreaddons_add_plugin(solidautoeject SOURCES ${kded_solidautoeject_SRCS} INSTALL_NAMESPACE "kf5/kded")
target_link_libraries(solidautoeject KF5::Solid KF5::DBusAddons KF5::CoreAddons)
