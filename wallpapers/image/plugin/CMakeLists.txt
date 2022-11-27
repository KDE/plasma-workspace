set(image_SRCS
    imagebackend.cpp
    slidemodel.cpp
    slidefiltermodel.cpp
    sortingmode.h
    finder/distance.cpp
    finder/findsymlinktarget.h
    finder/imagefinder.cpp
    finder/mediametadatafinder.cpp
    finder/suffixcheck.cpp
    finder/packagefinder.cpp
    model/abstractimagelistmodel.cpp
    model/imageroles.h
    model/packagelistmodel.cpp
    model/imagelistmodel.cpp
    model/imageproxymodel.cpp
    provider/packageimageprovider.cpp
    provider/providertype.h
    utils/backgroundtype.h
    utils/maximizedwindowmonitor.cpp
    utils/mediaproxy.cpp
)

ecm_qt_declare_logging_category(image_SRCS HEADER debug.h
                                           IDENTIFIER IMAGEWALLPAPER
                                           CATEGORY_NAME kde.wallpapers.image
                                           DEFAULT_SEVERITY Info)

add_library(plasma_wallpaper_imageplugin_static STATIC ${image_SRCS})

set_property(TARGET plasma_wallpaper_imageplugin_static PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(plasma_wallpaper_imageplugin_static
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
    KF5::Package
    PW::LibTaskManager
    )

if(HAVE_KF5KExiv2)
    target_link_libraries(plasma_wallpaper_imageplugin_static KF5::KExiv2)
endif()
configure_file(config-KF5KExiv2.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-KF5KExiv2.h)

add_library(plasma_wallpaper_imageplugin SHARED imageplugin.cpp)
target_link_libraries(plasma_wallpaper_imageplugin
    plasma_wallpaper_imageplugin_static
)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
