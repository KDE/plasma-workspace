
add_subdirectory(systemmonitor)

# Systemmonitor presets
plasma_install_package(coreusage org.kde.plasma.systemmonitor.cpucore)
plasma_install_package(cpu org.kde.plasma.systemmonitor.cpu)
plasma_install_package(memory org.kde.plasma.systemmonitor.memory)
plasma_install_package(diskusage org.kde.plasma.systemmonitor.diskusage)
plasma_install_package(diskactivity org.kde.plasma.systemmonitor.diskactivity)
plasma_install_package(net org.kde.plasma.systemmonitor.net)
