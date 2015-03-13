add_subdirectory(ions)

#set(weather_SRCS weatherengine.cpp)
#add_library(plasma_engine_weather MODULE ${weather_SRCS})
#target_link_libraries (plasma_engine_weather KF5::KIOCore KF5::Solid KF5::Plasma KF5::KDELibs4Support weather_ion)
#install (TARGETS plasma_engine_weather DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
#install (FILES plasma-dataengine-weather.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
