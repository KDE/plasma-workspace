# the Ion shared library
set (ionlib_SRCS ion.cpp)
ecm_qt_declare_logging_category(ionlib_SRCS
    HEADER iondebug.h
    IDENTIFIER IONENGINE
    CATEGORY_NAME kde.dataengine.ion
    DEFAULT_SEVERITY Info
)

add_library (weather_ion SHARED ${ionlib_SRCS})
generate_export_header(weather_ion BASE_NAME ion)
target_link_libraries (weather_ion PRIVATE KF5::I18n PUBLIC Qt::Core KF5::Plasma)

set_target_properties(weather_ion PROPERTIES
   VERSION 7.0.0
   SOVERSION 7
)

install (TARGETS weather_ion EXPORT kdeworkspaceLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install (FILES ion.h
               ${CMAKE_CURRENT_BINARY_DIR}/ion_export.h
         DESTINATION ${KDE_INSTALL_INCLUDEDIR}/plasma/weather COMPONENT Devel)

# install (FILES includes/Ion
#          DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KDE/Plasma/Weather COMPONENT Devel)

# the individual ion plugins
add_subdirectory(bbcukmet)
add_subdirectory(envcan)
add_subdirectory(noaa)
add_subdirectory(wetter.com)

