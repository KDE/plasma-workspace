set(favicons_engine_SRCS
    favicons.cpp
    faviconprovider.cpp
)

add_library(plasma_engine_favicons MODULE ${favicons_engine_SRCS} )
target_link_libraries(plasma_engine_favicons
    KF5::Plasma
    KF5::KIOCore
)

kservice_desktop_to_json(plasma_engine_favicons plasma-dataengine-favicons.desktop)

install(TARGETS plasma_engine_favicons DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine )
install(FILES plasma-dataengine-favicons.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


