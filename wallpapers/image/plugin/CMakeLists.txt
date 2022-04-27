set(image_SRCS
    imagebackend.cpp
    slidemodel.cpp
    slidefiltermodel.cpp
    sortingmode.h
    finder/imagesizefinder.cpp
    finder/distance.cpp
    finder/findsymlinktarget.h
    finder/imagefinder.cpp
    finder/suffixcheck.cpp
    finder/packagefinder.cpp
    finder/videofinder.cpp
    finder/videometadatafinder.cpp
    finder/xmlfinder.cpp
    model/abstractimagelistmodel.cpp
    model/imageroles.h
    model/packagelistmodel.cpp
    model/imagelistmodel.cpp
    model/imageproxymodel.cpp
    model/videolistmodel.cpp
    provider/packageimageprovider.cpp
    provider/providertype.h
    utils/backgroundtype.h
    utils/desktoppool.cpp
    utils/maximizedwindowmonitor.cpp
    utils/mediaproxy.cpp
    utils/wideimage.cpp
)

ecm_qt_declare_logging_category(image_SRCS HEADER debug.h
                                           IDENTIFIER IMAGEWALLPAPER
                                           CATEGORY_NAME kde.wallpapers.image
                                           DEFAULT_SEVERITY Info)

add_library(plasma_wallpaper_imageplugin_static STATIC ${image_SRCS})

target_link_libraries(plasma_wallpaper_imageplugin_static
    Qt::Core
    Qt::Multimedia # Video background
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
    PW::LibTaskManager
    )

add_library(plasma_wallpaper_imageplugin SHARED imageplugin.cpp)
target_link_libraries(plasma_wallpaper_imageplugin
    plasma_wallpaper_imageplugin_static
)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
