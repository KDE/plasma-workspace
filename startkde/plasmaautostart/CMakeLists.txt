add_library(PlasmaAutostart STATIC plasmaautostart.cpp)
set_property(TARGET PlasmaAutostart PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(PlasmaAutostart KF5::CoreAddons KF5::ConfigCore)
