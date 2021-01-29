if(NOT CMAKE_VERSION VERSION_LESS "3.10.0")
    # CMake 3.9+ warns about automoc on files without Q_OBJECT, and doesn't know about other macros.
    # 3.10+ lets us provide more macro names that require automoc.
    list(APPEND CMAKE_AUTOMOC_MACRO_NAMES K_EXPORT_PLASMA_GEOLOCATIONPROVIDER)
endif()

set(plasma_geolocation_interface_SRCS geolocationprovider.cpp)
add_library(plasma-geolocation-interface SHARED ${plasma_geolocation_interface_SRCS})
target_link_libraries(plasma-geolocation-interface
    PUBLIC
        Qt::Core
        Qt::Network
        KF5::Plasma
    PRIVATE
        KF5::KIOCore
)
set_target_properties(plasma-geolocation-interface PROPERTIES
   VERSION ${PROJECT_VERSION}
   SOVERSION ${PROJECT_VERSION_MAJOR}
)
install(TARGETS plasma-geolocation-interface ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES geolocationprovider.h ${CMAKE_CURRENT_BINARY_DIR}/geolocation_export.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/plasma/geolocation
        COMPONENT Devel)

#install(FILES includes/Interface
#        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KDE/Plasma/Geolocation
#        COMPONENT Devel)
# -------------------------------------------------------------------------------------------------


add_library(plasma_engine_geolocation MODULE geolocation.cpp)
target_compile_definitions(plasma_engine_geolocation PRIVATE -DQT_NO_KEYWORDS)
generate_export_header(plasma_engine_geolocation EXPORT_FILE_NAME "geolocation_export.h" BASE_NAME "GEOLOCATION")
target_link_libraries(plasma_engine_geolocation
    plasma-geolocation-interface
    KF5::Plasma
    KF5::CoreAddons
    KF5::KIOCore
    KF5::NetworkManagerQt
    KF5::Service
    KF5::Solid)
kcoreaddons_desktop_to_json(plasma_engine_geolocation plasma-dataengine-geolocation.desktop)

install(TARGETS plasma_engine_geolocation DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-geolocation.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES plasma-geolocationprovider.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

# -------------------------------------------------------------------------------------------------

set(plasma_geolocation_ip_SRCS location_ip.cpp)
ecm_qt_declare_logging_category(plasma_geolocation_ip_SRCS HEADER geolocdebug.h IDENTIFIER DATAENGINE_GEOLOCATION CATEGORY_NAME org.kde.plasma.dataengine.geolocation)
add_library(plasma-geolocation-ip MODULE ${plasma_geolocation_ip_SRCS})
target_compile_definitions(plasma-geolocation-ip PRIVATE -DQT_NO_KEYWORDS)
target_link_libraries(plasma-geolocation-ip plasma-geolocation-interface KF5::KIOCore KF5::NetworkManagerQt)
install(FILES plasma-geolocation-ip.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(TARGETS plasma-geolocation-ip DESTINATION ${KDE_INSTALL_PLUGINDIR})

# -------------------------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
find_package(libgps)
set_package_properties(libgps PROPERTIES DESCRIPTION "GPS support for geolocation"
                       URL "https://www.berlios.de/software/gpsd/"
                       TYPE OPTIONAL
                      )
if(LIBGPS_FOUND)
    include_directories(${LIBGPS_INCLUDES} ${LIBGPS_INCLUDE_DIR})
    set(plasma_geolocation_gps_SRCS location_gps.cpp)
    ecm_qt_declare_logging_category(plasma_geolocation_gps_SRCS HEADER geolocdebug.h IDENTIFIER DATAENGINE_GEOLOCATION CATEGORY_NAME org.kde.plasma.dataengine.geolocation)
    add_library(plasma-geolocation-gps MODULE ${plasma_geolocation_gps_SRCS})
    target_link_libraries(plasma-geolocation-gps plasma-geolocation-interface ${LIBGPS_LIBRARIES})
    install(FILES plasma-geolocation-gps.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
    install(TARGETS plasma-geolocation-gps DESTINATION ${KDE_INSTALL_PLUGINDIR})
endif()

# -------------------------------------------------------------------------------------------------
