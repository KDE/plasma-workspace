# the Ion shared library
set (ionlib_SRCS ion.cpp)
add_library (weather_ion SHARED ${ionlib_SRCS})
target_link_libraries (weather_ion KF5::KIOCore KF5::Plasma KF5::I18n KF5::KDELibs4Support)
target_link_libraries (weather_ion LINK_INTERFACE_LIBRARIES KF5::KIOCore KF5::Plasma)

set_target_properties(weather_ion PROPERTIES
   VERSION 6.0.0
   SOVERSION 6
)

install (TARGETS weather_ion EXPORT kdeworkspaceLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install (FILES ion.h
               ion_export.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/plasma/weather COMPONENT Devel)

install (FILES includes/Ion
         DESTINATION ${INCLUDE_INSTALL_DIR}/KDE/Plasma/Weather COMPONENT Devel)

# the individual ion plugins
#add_subdirectory(bbcukmet)
#add_subdirectory(envcan)
add_subdirectory(noaa)
#add_subdirectory(wetter.com)
#add_subdirectory(debianweather)

