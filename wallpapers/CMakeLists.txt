
# Where the wallpaper to fall back on comes from. Kept out of libkworkspace, whose other users
# would otherwise link Plasma, and with it the QML runtime, to reach session management.
add_library(plasma_defaultwallpaper STATIC defaultwallpaper.cpp defaultwallpaper.h)
set_property(TARGET plasma_defaultwallpaper PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(plasma_defaultwallpaper PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(plasma_defaultwallpaper
    PUBLIC
        KF6::Package
    PRIVATE
        KF6::ConfigCore
        Plasma::Plasma
)

add_subdirectory(image)
plasma_install_package(color org.kde.color wallpapers wallpaper)
