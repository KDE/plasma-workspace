project(plasma-wallpaper-image)

set(image_SRCS
    image.cpp
    imageplugin.cpp
    wallpaperpackage.cpp
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
	 KF5::ThreadWeaver
         KF5::I18n
    KF5::KIOCore
    KF5::KIOWidgets # KFileDialog
	 KF5::NewStuff
         KF5::KDE4Support
         )

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/wallpapers/image)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/wallpapers/image)

install(FILES wallpaper.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

plasma_install_package(imagepackage org.kde.image plasma/wallpapers wallpaper)
plasma_install_package(slideshowpackage org.kde.slideshow plasma/wallpapers wallpaper)

install(DIRECTORY imagepackage/contents/ui DESTINATION ${DATA_INSTALL_DIR}/plasma/wallpapers/org.kde.slideshow/contents PATTERN .svn EXCLUDE PATTERN CMakeLists.txt EXCLUDE PATTERN Messages.sh EXCLUDE)