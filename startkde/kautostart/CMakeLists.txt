add_library(PlasmaKAutostart STATIC kautostart.cpp)
target_link_libraries(PlasmaKAutostart KF5::CoreAddons KF5::ConfigCore)
