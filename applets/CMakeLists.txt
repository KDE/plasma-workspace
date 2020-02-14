plasma_install_package(activitybar org.kde.plasma.activitybar)
add_subdirectory(icon)

plasma_install_package(analog-clock org.kde.plasma.analogclock)
plasma_install_package(mediacontroller org.kde.plasma.mediacontroller)
plasma_install_package(lock_logout org.kde.plasma.lock_logout)

add_subdirectory(appmenu)
add_subdirectory(systemmonitor)
add_subdirectory(batterymonitor)
add_subdirectory(calendar)
add_subdirectory(devicenotifier)
add_subdirectory(digital-clock)
add_subdirectory(kicker)
add_subdirectory(panelspacer)

plasma_install_package(clipboard org.kde.plasma.clipboard)

if(NOT WIN32)
#    #notifications
#    #should compile also on windows? (even if doesn't really make sense)
    add_subdirectory(notifications)
    add_subdirectory(systemtray)
endif()
