add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_share\")

set(share_engine_SRCS
        shareprovider.cpp
        shareengine.cpp
        shareservice.cpp)

add_library(plasma_engine_share MODULE ${share_engine_SRCS})
target_link_libraries(plasma_engine_share
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::JsEmbed
)
kcoreaddons_desktop_to_json(plasma_engine_share plasma-dataengine-share.desktop)

install(TARGETS plasma_engine_share
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

install(FILES plasma-dataengine-share.desktop
        DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES data/plasma_shareprovider.desktop
        DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

install(FILES share.operations
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

add_subdirectory(packagestructure)
add_subdirectory(backends)
