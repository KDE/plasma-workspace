add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.image\")

set(image_SRCS
    image.cpp
    imageplugin.cpp
    backgroundlistmodel.cpp
)

add_library(plasma_wallpaper_imageplugin SHARED ${image_SRCS})

target_link_libraries(plasma_wallpaper_imageplugin
	 Qt5::Core
	 Qt5::Quick
	 Qt5::Qml
	 KF5::Plasma
         KF5::KIOCore
	 KF5::KIOWidgets
         KF5::I18n
         KF5::KIOCore
         KF5::KIOWidgets # KFileDialog
         KF5::NewStuff
         KF5::GuiAddons
         )

add_subdirectory(autotests)

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)

install(FILES wallpaper.knsrc DESTINATION ${KDE_INSTALL_CONFDIR})

plasma_install_package(imagepackage org.kde.image wallpapers wallpaper)
plasma_install_package(slideshowpackage org.kde.slideshow wallpapers wallpaper)

install(DIRECTORY imagepackage/contents/ui DESTINATION ${PLASMA_DATA_INSTALL_DIR}/wallpapers/org.kde.slideshow/contents PATTERN .svn EXCLUDE PATTERN CMakeLists.txt EXCLUDE PATTERN Messages.sh EXCLUDE)
