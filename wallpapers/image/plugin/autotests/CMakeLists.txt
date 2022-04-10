include(ECMMarkAsTest)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ImageSizeFinder test
ecm_add_test(test_imagesizefinder.cpp TEST_NAME testimagesizefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageFinder test
ecm_add_test(test_imagefinder.cpp TEST_NAME testimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# PackageFinder test
ecm_add_test(test_packagefinder.cpp TEST_NAME testpackageimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageListModel test
ecm_add_test(test_imagelistmodel.cpp TEST_NAME testimagelistmodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# PackageListModel test
ecm_add_test(test_packagelistmodel.cpp TEST_NAME testpackageimagelistmodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)

# ImageProxyModel test
ecm_add_test(test_imageproxymodel.cpp TEST_NAME testimageproxymodel
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)
