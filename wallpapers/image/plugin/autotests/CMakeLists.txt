include(ECMMarkAsTest)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(testfindpreferredimage_SRCS
    testfindpreferredimage.cpp
    ../imagebackend.cpp
    ../backgroundlistmodel.cpp
    )

add_executable(testfindpreferredimage EXCLUDE_FROM_ALL ${testfindpreferredimage_SRCS})

target_link_libraries(testfindpreferredimage
	 plasma_wallpaper_imageplugin
	 Qt::Test)

# ImageSizeFinder test
ecm_add_test(test_imagesizefinder.cpp TEST_NAME testimagesizefinder
    LINK_LIBRARIES Qt::Test plasma_wallpaper_imageplugin_static)
