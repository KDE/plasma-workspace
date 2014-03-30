project(plasma-system-monitor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} )

# lib

set(lib_SRCS
    monitoricon.cpp
    applet.cpp
    plotter.cpp
)
add_library(plasma_applet-system-monitor SHARED ${lib_SRCS})
target_link_libraries(plasma_applet-system-monitor KF5::Plasma ${KDE4_KDEUI_LIBS})
set_target_properties(plasma_applet-system-monitor PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

# HDD Space Monitor

set(hdd_SRCS
    hdd.cpp
)
kde4_add_ui_files(hdd_SRCS hdd-config.ui)
add_library(plasma_applet_sm_hdd MODULE ${hdd_SRCS})
target_link_libraries(plasma_applet_sm_hdd KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# HDD Activity Monitor

set(hdd_activity_SRCS
    hdd_activity.cpp
)
kde4_add_ui_files(hdd_activity_SRCS hdd_activity-config.ui)
add_library(plasma_applet_sm_hdd_activity MODULE ${hdd_activity_SRCS})
target_link_libraries(plasma_applet_sm_hdd_activity KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# Temperature Monitor

set(temparature_SRCS
    temperature.cpp
    temperature-offset-delegate.cpp
)
kde4_add_ui_files(temparature_SRCS temperature-config.ui)
add_library(plasma_applet_sm_temperature MODULE ${temparature_SRCS})
target_link_libraries(plasma_applet_sm_temperature KF5::Plasma ${KDE4_KDEUI_LIBS} ${KDE4_KUNITCONVERSION_LIBS} plasma_applet-system-monitor)

# Network Monitor

set(net_SRCS
    net.cpp
)
kde4_add_ui_files(net_SRCS net-config.ui)
add_library(plasma_applet_sm_net MODULE ${net_SRCS})
target_link_libraries(plasma_applet_sm_net KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# CPU Monitor

set(cpu_SRCS
    cpu.cpp
)
kde4_add_ui_files(cpu_SRCS cpu-config.ui)
add_library(plasma_applet_sm_cpu MODULE ${cpu_SRCS})
target_link_libraries(plasma_applet_sm_cpu KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# Harware Info

set(hwinfo_SRCS
    hwinfo.cpp
)
add_library(plasma_applet_sm_hwinfo MODULE ${hwinfo_SRCS})
target_link_libraries(plasma_applet_sm_hwinfo KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# Ram Monitor

set(ram_SRCS
    ram.cpp
)
kde4_add_ui_files(ram_SRCS ram-config.ui)
add_library(plasma_applet_sm_ram MODULE ${ram_SRCS})
target_link_libraries(plasma_applet_sm_ram KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# Main applet

set(system-monitor_SRCS
    system-monitor.cpp
    monitorbutton.cpp
)
add_library(plasma_applet_system-monitor MODULE ${system-monitor_SRCS})
target_link_libraries(plasma_applet_system-monitor KF5::Plasma ${KDE4_KDEUI_LIBS} plasma_applet-system-monitor)

# Install
install(TARGETS
    plasma_applet-system-monitor
    ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS
    plasma_applet_sm_hdd_activity
    plasma_applet_sm_hdd
    plasma_applet_sm_temperature
    plasma_applet_sm_hwinfo
    plasma_applet_sm_net
    plasma_applet_sm_cpu
    plasma_applet_sm_ram
    plasma_applet_system-monitor
    DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES
    plasma-applet-sm_hdd_activity.desktop
    plasma-applet-sm_hdd.desktop
    plasma-applet-sm_temperature.desktop
    plasma-applet-sm_hwinfo.desktop
    plasma-applet-sm_net.desktop
    plasma-applet-sm_cpu.desktop
    plasma-applet-sm_ram.desktop
    plasma-applet-system-monitor.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES
    hdd_panel.svgz
    DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/system-monitor/)
