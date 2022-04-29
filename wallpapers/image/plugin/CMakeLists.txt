set(image_SRCS
    desktoppool.cpp
    imagebackend.cpp
    slidemodel.cpp
    slidefiltermodel.cpp
    sortingmode.h
    clockskewnotifier/clockskewnotifierengine.cpp
    finder/imagesizefinder.cpp
    finder/distance.cpp
    finder/findsymlinktarget.h
    finder/imagefinder.cpp
    finder/suffixcheck.cpp
    finder/packagefinder.cpp
    finder/xmlfinder.cpp
    model/abstractimagelistmodel.cpp
    model/imageroles.h
    model/packagelistmodel.cpp
    model/imagelistmodel.cpp
    model/imageproxymodel.cpp
    model/xmlimagelistmodel.cpp
    model/xmlpreviewgenerator.cpp
    provider/packageimageprovider.cpp
    provider/wideimageprovider.cpp
)

ecm_qt_declare_logging_category(image_SRCS HEADER debug.h
                                           IDENTIFIER IMAGEWALLPAPER
                                           CATEGORY_NAME kde.wallpapers.image
                                           DEFAULT_SEVERITY Info)

add_library(plasma_wallpaper_imageplugin_static STATIC)
target_sources(plasma_wallpaper_imageplugin_static PRIVATE ${image_SRCS})

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_sources(plasma_wallpaper_imageplugin_static PRIVATE clockskewnotifier/clockskewnotifierengine_linux.cpp)
endif()

target_link_libraries(plasma_wallpaper_imageplugin_static
    Qt::Core
    Qt::Gui # Wide wallpaper support
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

add_library(plasma_wallpaper_imageplugin SHARED imageplugin.cpp)
target_link_libraries(plasma_wallpaper_imageplugin
    plasma_wallpaper_imageplugin_static
)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
