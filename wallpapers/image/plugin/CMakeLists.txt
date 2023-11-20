set(image_SRCS
    imagebackend.cpp
    slidemodel.cpp
    slidefiltermodel.cpp
    sortingmode.h
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
                                           DEFAULT_SEVERITY Info
                                           DESCRIPTION "plasma wallpapers image"
                                           EXPORT PLASMAWORKSPACE
                                       )

add_library(plasma_wallpaper_imageplugin_static STATIC ${image_SRCS})

set_property(TARGET plasma_wallpaper_imageplugin_static PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(plasma_wallpaper_imageplugin_static
    Qt::Core
    Qt::Quick
    Qt::Qml
    KF6::Plasma
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

add_library(plasma_wallpaper_imageplugin SHARED imageplugin.cpp)
target_link_libraries(plasma_wallpaper_imageplugin
    plasma_wallpaper_imageplugin_static
)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

install(TARGETS plasma_wallpaper_imageplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/wallpapers/image)
