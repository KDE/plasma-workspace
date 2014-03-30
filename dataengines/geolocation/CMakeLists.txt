project(geolocation)

# -------------------------------------------------------------------------------------------------

set(plasma_geolocation_interface_SRCS geolocationprovider.cpp)
add_library(plasma-geolocation-interface SHARED ${plasma_geolocation_interface_SRCS})
target_link_libraries(plasma-geolocation-interface
    PUBLIC
        Qt5::Core
        KF5::Plasma
    PRIVATE
        KF5::KIOCore
)
set_target_properties(plasma-geolocation-interface PROPERTIES
   VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)
install(TARGETS plasma-geolocation-interface ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES geolocationprovider.h ${CMAKE_CURRENT_BINARY_DIR}/geolocation_export.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/plasma/geolocation
        COMPONENT Devel)

#install(FILES includes/Interface
#        DESTINATION ${INCLUDE_INSTALL_DIR}/KDE/Plasma/Geolocation
#        COMPONENT Devel)
# -------------------------------------------------------------------------------------------------


add_library(plasma_engine_geolocation MODULE geolocation.cpp)
generate_export_header(plasma_engine_geolocation EXPORT_FILE_NAME "geolocation_export.h" BASE_NAME "GEOLOCATION")
target_link_libraries(plasma_engine_geolocation
    plasma-geolocation-interface
    KF5::Plasma
    KF5::CoreAddons
    KF5::KIOCore
    KF5::Service
    KF5::Solid)
kservice_desktop_to_json(plasma_engine_geolocation plasma-dataengine-geolocation.desktop)

install(TARGETS plasma_engine_geolocation DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-geolocation.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES plasma-geolocationprovider.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

# -------------------------------------------------------------------------------------------------

set(plasma_geolocation_ip_SRCS location_ip.cpp)
add_library(plasma-geolocation-ip MODULE ${plasma_geolocation_ip_SRCS})
target_link_libraries(plasma-geolocation-ip plasma-geolocation-interface KF5::KIOCore)
install(FILES plasma-geolocation-ip.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS plasma-geolocation-ip DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/geolocationprovider)

# -------------------------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
find_package(libgps)
set_package_properties(libgps PROPERTIES DESCRIPTION "GPS support for geolocation"
                       URL "http://gpsd.berlios.de/"
                       TYPE OPTIONAL
                      )
if(LIBGPS_FOUND)
    include_directories(${LIBGPS_INCLUDES} ${LIBGPS_INCLUDE_DIR})
    set(plasma_geolocation_gps_SRCS location_gps.cpp)
    add_library(plasma-geolocation-gps MODULE ${plasma_geolocation_gps_SRCS})
    target_link_libraries(plasma-geolocation-gps plasma-geolocation-interface ${LIBGPS_LIBRARIES})
    install(FILES plasma-geolocation-gps.desktop DESTINATION ${SERVICES_INSTALL_DIR})
    install(TARGETS plasma-geolocation-gps DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/geolocationprovider)
endif()

# -------------------------------------------------------------------------------------------------
