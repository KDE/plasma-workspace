########### next target ###############

set(kded_solidautoeject_SRCS
   solidautoeject.cpp
)

add_library(kded_solidautoeject MODULE ${kded_solidautoeject_SRCS})

target_link_libraries(kded_solidautoeject KF5::Solid KF5::DBusAddons KF5::KDE4Support)

install(TARGETS kded_solidautoeject DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES solidautoeject.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

