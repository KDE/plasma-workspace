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

kcoreaddons_add_plugin(plasma_engine_geolocation SOURCES geolocation.cpp INSTALL_NAMESPACE plasma/dataengine)
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

kcoreaddons_add_plugin(plasma-geolocation-ip SOURCES location_ip.cpp INSTALL_NAMESPACE plasma/geolocationprovider)
ecm_qt_declare_logging_category(plasma-geolocation-ip HEADER geolocdebug.h IDENTIFIER DATAENGINE_GEOLOCATION CATEGORY_NAME org.kde.plasma.dataengine.geolocation)
kcoreaddons_desktop_to_json(plasma-geolocation-ip plasma-geolocation-ip.desktop)
target_compile_definitions(plasma-geolocation-ip PRIVATE -DQT_NO_KEYWORDS)
target_link_libraries(plasma-geolocation-ip plasma-geolocation-interface KF5::KIOCore KF5::NetworkManagerQt)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
find_package(libgps)
set_package_properties(libgps PROPERTIES DESCRIPTION "GPS support for geolocation"
                       URL "https://www.berlios.de/software/gpsd/"
                       TYPE OPTIONAL
                      )
if(LIBGPS_FOUND)
    include_directories(${LIBGPS_INCLUDES} ${LIBGPS_INCLUDE_DIR})
    kcoreaddons_add_plugin(plasma-geolocation-gps SOURCES location_gps.cpp INSTALL_NAMESPACE plasma/geolocationprovider)
    ecm_qt_declare_logging_category(plasma-geolocation-gps HEADER geolocdebug.h IDENTIFIER DATAENGINE_GEOLOCATION CATEGORY_NAME org.kde.plasma.dataengine.geolocation)
    kcoreaddons_desktop_to_json(plasma-geolocation-gps plasma-geolocation-gps.desktop)
    target_link_libraries(plasma-geolocation-gps plasma-geolocation-interface ${LIBGPS_LIBRARIES})
endif()
