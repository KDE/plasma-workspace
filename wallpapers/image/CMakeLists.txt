add_definitions(-DTRANSLATION_DOMAIN=\"plasma_wallpaper_org.kde.image\")

set(image_SRCS
    image.cpp
    imageplugin.cpp
    backgroundlistmodel.cpp
    slidemodel.cpp
    slidefiltermodel.cpp
)

ecm_qt_declare_logging_category(image_SRCS HEADER debug.h
										   IDENTIFIER IMAGEWALLPAPER
										   CATEGORY_NAME kde.wallpapers.image
										   DEFAULT_SEVERITY Info)

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
         )

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)

configure_file(wallpaper.knsrc.cmake ${CMAKE_CURRENT_BINARY_DIR}/wallpaper.knsrc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wallpaper.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

plasma_install_package(imagepackage org.kde.image wallpapers wallpaper)
plasma_install_package(slideshowpackage org.kde.slideshow wallpapers wallpaper)

install(DIRECTORY imagepackage/contents/ui DESTINATION ${PLASMA_DATA_INSTALL_DIR}/wallpapers/org.kde.slideshow/contents PATTERN .svn EXCLUDE PATTERN CMakeLists.txt EXCLUDE PATTERN Messages.sh EXCLUDE)
