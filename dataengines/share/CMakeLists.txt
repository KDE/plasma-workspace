project(shareengine)

set(share_engine_SRCS
        shareprovider.cpp
        shareengine.cpp
        shareservice.cpp
        share_package.cpp)

add_library(plasma_engine_share MODULE ${share_engine_SRCS})
target_link_libraries(plasma_engine_share
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::JsEmbed
                      KF5::KDELibs4Support
)
kservice_desktop_to_json(plasma_engine_share plasma-dataengine-share.desktop)

install(TARGETS plasma_engine_share
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/dataengine)

install(FILES plasma-dataengine-share.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES data/plasma_shareprovider.desktop
        DESTINATION ${SERVICETYPES_INSTALL_DIR})

install(FILES share.operations
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

set(sharepackage_SRCS
        share_package.cpp
        plugin_share_package.cpp
)

add_subdirectory(backends)

add_library(plasma_packagestructure_share MODULE ${sharepackage_SRCS})
target_link_libraries(plasma_packagestructure_share
        KF5::Plasma
        KF5::I18n
        KF5::Service)

install(TARGETS plasma_packagestructure_share
                 DESTINATION ${PLASMA_DATA_INSTALL_DIR}/packagestructure)
install(FILES data/plasma-packagestructure-share.desktop
               DESTINATION ${SERVICES_INSTALL_DIR})
