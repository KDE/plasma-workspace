add_executable(autostart_test
    autostarttest.cpp
    ../autostart.cpp
)

target_link_libraries(autostart_test
    Qt5::Core
    KF5::Service
)
