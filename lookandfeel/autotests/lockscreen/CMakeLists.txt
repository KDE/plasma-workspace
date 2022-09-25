find_package(Qt${QT_MAJOR_VERSION}QuickTest ${REQUIRED_QT_VERSION} CONFIG QUIET)

if(NOT TARGET Qt::QuickTest)
    message(STATUS "Qt5QuickTest not found, autotests will not be built.")
    return()
endif()


add_executable(lockscreentest main.cpp)
target_link_libraries(lockscreentest PRIVATE Qt::QuickTest Qt::Quick)

add_test(NAME testlockscreen
    COMMAND lockscreentest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
