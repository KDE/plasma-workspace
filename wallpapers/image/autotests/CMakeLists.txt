set(testfindpreferredimage_SRCS
    testfindpreferredimage.cpp
    ../image.cpp
    ../backgroundlistmodel.cpp
    )

add_executable(testfindpreferredimage EXCLUDE_FROM_ALL ${testfindpreferredimage_SRCS})

target_link_libraries(testfindpreferredimage
	 plasma_wallpaper_imageplugin
	 Qt::Test)
