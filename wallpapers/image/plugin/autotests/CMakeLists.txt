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
set(testimagesizefinder_SRCS
    test_imagesizefinder.cpp
    ../finder/imagesizefinder.cpp
)

add_executable(testimagesizefinder ${testimagesizefinder_SRCS})

target_link_libraries(testimagesizefinder
    plasma_wallpaper_imageplugin
    Qt::Test
)

ecm_mark_as_test(testimagesizefinder)
