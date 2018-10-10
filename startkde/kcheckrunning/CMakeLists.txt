set(kcheckrunning_SRCS
    kcheckrunning.cpp)

add_executable( kcheckrunning ${kcheckrunning_SRCS})

target_link_libraries(kcheckrunning ${X11_LIBRARIES})
target_include_directories(kcheckrunning PRIVATE ${X11_X11_INCLUDE_PATH})

install(TARGETS kcheckrunning ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
