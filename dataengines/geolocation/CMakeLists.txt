remove_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x50f02)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00) # needed for QNetworkConfigurationManager

set(plasma_geolocation_interface_SRCS geolocationprovider.cpp)
add_library(plasma-geolocation-interface SHARED ${plasma_geolocation_interface_SRCS})
target_link_libraries(plasma-geolocation-interface
    PUBLIC
        Qt::Core
        Qt::Network
        KF6::Plasma
        KF6::Plasma5Support
    PRIVATE
        KF6::KIOCore
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
    KF6::Plasma
    KF6::Plasma5Support
    KF6::CoreAddons
    KF6::KIOCore
    KF6::NetworkManagerQt
    KF6::Service
    KF6::Solid)

kcoreaddons_add_plugin(plasma-geolocation-ip SOURCES location_ip.cpp INSTALL_NAMESPACE plasma5support/geolocationprovider)
ecm_qt_declare_logging_category(plasma-geolocation-ip HEADER geolocdebug.h IDENTIFIER DATAENGINE_GEOLOCATION CATEGORY_NAME org.kde.plasma.dataengine.geolocation)
target_compile_definitions(plasma-geolocation-ip PRIVATE -DQT_NO_KEYWORDS)
target_link_libraries(plasma-geolocation-ip plasma-geolocation-interface KF6::NetworkManagerQt)

pkg_check_modules(LIBGPS libgps IMPORTED_TARGET)

if(TARGET PkgConfig::LIBGPS)
    kcoreaddons_add_plugin(plasma-geolocation-gps SOURCES location_gps.cpp INSTALL_NAMESPACE plasma5support/geolocationprovider)
    ecm_qt_declare_logging_category(plasma-geolocation-gps HEADER geolocdebug.h IDENTIFIER DATAENGINE_GEOLOCATION CATEGORY_NAME org.kde.plasma.dataengine.geolocation)
    target_link_libraries(plasma-geolocation-gps plasma-geolocation-interface PkgConfig::LIBGPS)
endif()
