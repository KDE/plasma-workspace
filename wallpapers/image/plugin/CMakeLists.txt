set(image_SRCS
    imagebackend.cpp
    imageplugin.cpp
    backgroundlistmodel.cpp
    slidemodel.cpp
    slidefiltermodel.cpp
    sortingmode.h
    finder/imagesizefinder.cpp
)

ecm_qt_declare_logging_category(image_SRCS HEADER debug.h
                                           IDENTIFIER IMAGEWALLPAPER
                                           CATEGORY_NAME kde.wallpapers.image
                                           DEFAULT_SEVERITY Info)

add_library(plasma_wallpaper_imageplugin SHARED ${image_SRCS})

target_link_libraries(plasma_wallpaper_imageplugin
    Qt::Core
    Qt::Quick
    Qt::Qml
    KF5::Plasma
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::KIOCore
    KF5::KIOGui
    KF5::NewStuff
    KF5::Notifications
    )

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
