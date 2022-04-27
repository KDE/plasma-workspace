include(ECMMarkAsTest)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS QuickTest)

# ImageSizeFinder test
ecm_add_test(test_imagesizefinder.cpp TEST_NAME testimagesizefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageFinder test
ecm_add_test(test_imagefinder.cpp TEST_NAME testimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# PackageFinder test
ecm_add_test(test_packagefinder.cpp TEST_NAME testpackageimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# XmlFinder test
ecm_add_test(test_xmlfinder.cpp TEST_NAME testxmlimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageListModel test
ecm_add_test(test_imagelistmodel.cpp TEST_NAME testimagelistmodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# PackageListModel test
ecm_add_test(test_packagelistmodel.cpp TEST_NAME testpackageimagelistmodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# XmlImageListModel test
ecm_add_test(test_xmlimagelistmodel.cpp TEST_NAME testxmlimagelistmodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageProxyModel test
ecm_add_test(test_imageproxymodel.cpp TEST_NAME testimageproxymodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# SlideModel test
ecm_add_test(test_slidemodel.cpp TEST_NAME testslidemodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# SlideFilterModel test
ecm_add_test(test_slidefiltermodel.cpp TEST_NAME testslidefiltermodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageBackend test
add_executable(testimagebackend tst_imagebackend.cpp)
target_link_libraries(testimagebackend
    Qt::Qml
    Qt::QuickTest
    Qt::Test
)
add_test(NAME testimagebackend
    COMMAND testimagebackend
    -import ${CMAKE_BINARY_DIR}/bin
    -input tst_imagebackend.qml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# WideImageProvider test
add_test(NAME testwideimageprovider
    COMMAND testimagebackend
    -import ${CMAKE_BINARY_DIR}/bin
    -input tst_393781_wideimage.qml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
