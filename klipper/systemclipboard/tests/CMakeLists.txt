add_executable(pasteclient paste.cpp)

target_link_libraries(pasteclient
    systemclipboard
)
