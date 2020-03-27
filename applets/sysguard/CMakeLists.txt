
add_subdirectory(sensorchart)

#sensorchart presets
plasma_install_package(coreusage org.kde.ksysguard.sensorchart.cpucore)
plasma_install_package(applicationtable org.kde.ksysguard.sensorchart.applicationtable)
plasma_install_package(cpuusage org.kde.ksysguard.sensorchart.cpuusage)
plasma_install_package(memoryusage org.kde.ksysguard.sensorchart.memoryusage)
plasma_install_package(diskusage org.kde.ksysguard.sensorchart.diskusage)
plasma_install_package(networkspeed org.kde.ksysguard.sensorchart.networkspeed)
