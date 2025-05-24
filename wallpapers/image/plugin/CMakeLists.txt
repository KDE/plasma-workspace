set(image_SRCS
    daynightwallpaper.cpp daynightwallpaper.h
    imagebackend.cpp imagebackend.h
    slidemodel.cpp slidemodel.h
    slidefiltermodel.cpp slidefiltermodel.h
    sortingmode.h
    finder/findsymlinktarget.h
    finder/imagefinder.cpp finder/imagefinder.h
    finder/mediametadatafinder.cpp finder/mediametadatafinder.h
    finder/suffixcheck.cpp finder/suffixcheck.h
    finder/packagefinder.cpp finder/packagefinder.h
    model/abstractimagelistmodel.cpp model/abstractimagelistmodel.h
    model/imageroles.h
    model/packagelistmodel.cpp model/packagelistmodel.h
    model/imagelistmodel.cpp model/imagelistmodel.h
    model/imageproxymodel.cpp model/imageproxymodel.h
    provider/packageimageprovider.cpp
    provider/providertype.h
    provider/wallpaperpreviewimageprovider.cpp provider/wallpaperpreviewimageprovider.h
    utils/backgroundtype.h
    utils/maximizedwindowmonitor.cpp utils/maximizedwindowmonitor.h
    utils/mediaproxy.cpp utils/mediaproxy.h
    utils/wallpaperurl.cpp utils/wallpaperurl.h
)

ecm_qt_declare_logging_category(image_SRCS HEADER debug.h
                                           IDENTIFIER IMAGEWALLPAPER
                                           CATEGORY_NAME kde.wallpapers.image
                                           DEFAULT_SEVERITY Info
                                           DESCRIPTION "plasma wallpapers image"
                                           EXPORT PLASMAWORKSPACE
                                       )

add_library(plasma_wallpaper_imageplugin_static STATIC ${image_SRCS})

set_property(TARGET plasma_wallpaper_imageplugin_static PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(plasma_wallpaper_imageplugin_static
    Qt::Core
    Qt::Positioning
    Qt::Quick
    Qt::Qml
    Plasma::Plasma
    KF6::GlobalAccel
    KF6::Holidays
    KF6::I18n
    KF6::JobWidgets
    KF6::KIOCore
    KF6::KIOGui
    KF6::Notifications
    KF6::Package
    KF6::Svg
    PW::LibTaskManager
    PW::KWorkspace
    )

if(HAVE_KExiv2)
    target_link_libraries(plasma_wallpaper_imageplugin_static KExiv2Qt6)
endif()
configure_file(config-KExiv2.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-KExiv2.h)

add_library(plasma_wallpaper_imageplugin SHARED imageplugin.cpp imageplugin.h)
target_link_libraries(plasma_wallpaper_imageplugin
    plasma_wallpaper_imageplugin_static
)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
