########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"solidautoeject\")

set(kded_solidautoeject_SRCS
   solidautoeject.cpp
)

add_library(kded_solidautoeject MODULE ${kded_solidautoeject_SRCS})

target_link_libraries(kded_solidautoeject KF5::Solid KF5::DBusAddons KF5::CoreAddons)

install(TARGETS kded_solidautoeject DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install(FILES solidautoeject.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kded)
