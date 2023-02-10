add_executable(autostart_test
    autostarttest.cpp
    ../autostart.cpp
)

target_link_libraries(autostart_test
    Qt::Core
    KF6::Service
)
