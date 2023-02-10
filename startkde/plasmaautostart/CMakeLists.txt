add_library(PlasmaAutostart STATIC plasmaautostart.cpp)
set_property(TARGET PlasmaAutostart PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(PlasmaAutostart KF6::CoreAddons KF6::ConfigCore)
