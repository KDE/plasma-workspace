add_subdirectory(ions)

set(weather_SRCS weatherengine.cpp)
add_library(plasma_engine_weather MODULE ${weather_SRCS})
target_link_libraries (plasma_engine_weather KIOCore KF5::Solid KF5::Plasma weather_ion)
install (TARGETS plasma_engine_weather DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install (FILES plasma-dataengine-weather.desktop DESTINATION ${SERVICES_INSTALL_DIR})
