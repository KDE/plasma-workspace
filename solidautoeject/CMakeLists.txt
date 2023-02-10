########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"solidautoeject\")

set(kded_solidautoeject_SRCS
   solidautoeject.cpp
)

kcoreaddons_add_plugin(solidautoeject SOURCES ${kded_solidautoeject_SRCS} INSTALL_NAMESPACE "kf5/kded")
target_link_libraries(solidautoeject KF6::Solid KF6::DBusAddons KF6::CoreAddons)
