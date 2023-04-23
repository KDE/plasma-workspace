add_executable(lockscreentest main.cpp)
target_link_libraries(lockscreentest PRIVATE Qt::QuickTest Qt::Quick)

add_test(NAME testlockscreen
    COMMAND lockscreentest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
