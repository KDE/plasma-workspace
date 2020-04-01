add_library(sensorapplet_packagestructure MODULE sensorappletpackage.cpp)

target_link_libraries(sensorapplet_packagestructure PRIVATE KF5::Package KF5::I18n)

install(TARGETS sensorapplet_packagestructure DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
