include(ECMMarkAsTest)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ImageFinder test
ecm_add_test(test_imagefinder.cpp TEST_NAME testimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)
set_tests_properties(testimagefinder PROPERTIES RUN_SERIAL ON)

# MediaMetadataFinder test
ecm_add_test(test_mediametadatafinder.cpp TEST_NAME testmediametadatafinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)
set_tests_properties(testmediametadatafinder PROPERTIES RUN_SERIAL ON)

# PackageFinder test
ecm_add_test(test_packagefinder.cpp TEST_NAME testpackageimagefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)
set_tests_properties(testpackageimagefinder PROPERTIES RUN_SERIAL ON)

# ImageListModel test
ecm_add_test(test_imagelistmodel.cpp TEST_NAME testimagelistmodel
    LINK_LIBRARIES Qt::Test PW::KWorkspace plasma_wallpaper_imageplugin_static)
set_tests_properties(testimagelistmodel PROPERTIES RUN_SERIAL ON)

# PackageListModel test
ecm_add_test(test_packagelistmodel.cpp TEST_NAME testpackageimagelistmodel
    LINK_LIBRARIES Qt::Test PW::KWorkspace plasma_wallpaper_imageplugin_static)
set_tests_properties(testpackageimagelistmodel PROPERTIES RUN_SERIAL ON)

# ImageProxyModel test
ecm_add_test(test_imageproxymodel.cpp TEST_NAME testimageproxymodel
    LINK_LIBRARIES Qt::Test PW::KWorkspace plasma_wallpaper_imageplugin_static)
set_tests_properties(testimageproxymodel PROPERTIES RUN_SERIAL ON)

# SlideModel test
ecm_add_test(test_slidemodel.cpp TEST_NAME testslidemodel
    LINK_LIBRARIES Qt::Test PW::KWorkspace plasma_wallpaper_imageplugin_static)
set_tests_properties(testslidemodel PROPERTIES RUN_SERIAL ON)

# SlideFilterModel test
ecm_add_test(test_slidefiltermodel.cpp TEST_NAME testslidefiltermodel
    LINK_LIBRARIES Qt::Test PW::KWorkspace plasma_wallpaper_imageplugin_static)
set_tests_properties(testslidefiltermodel PROPERTIES RUN_SERIAL ON)

# ImageBackend test
add_executable(testimagebackend tst_imagebackend.cpp)
target_link_libraries(testimagebackend
    Qt::Qml
    Qt::QuickTest
    Qt::Test
    PW::KWorkspace
)
add_test(NAME testimagebackend
    COMMAND testimagebackend
    -import ${CMAKE_BINARY_DIR}/bin
    -input tst_imagebackend.qml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(testimagebackend PROPERTIES RUN_SERIAL ON)

# ImageFrontend test
ecm_add_test(tst_imagefrontend.cpp TEST_NAME testimagefrontend
    LINK_LIBRARIES Qt::Qml Qt::QuickTest Qt::Test plasma_wallpaper_imageplugin_static)
set_tests_properties(testimagefrontend PROPERTIES RUN_SERIAL ON)
