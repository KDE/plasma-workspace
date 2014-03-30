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
                      KF5::KDE4Support
)
kservice_desktop_to_json(plasma_engine_share plasma-dataengine-share.desktop)

install(TARGETS plasma_engine_share
        DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)

install(FILES plasma-dataengine-share.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES data/plasma_shareprovider.desktop
        DESTINATION ${SERVICETYPES_INSTALL_DIR})

install(FILES share.operations
        DESTINATION ${DATA_INSTALL_DIR}/plasma/services)

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
                 DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/packagestructure)
install(FILES data/plasma-packagestructure-share.desktop
               DESTINATION ${SERVICES_INSTALL_DIR})
