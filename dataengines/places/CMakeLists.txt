set(places_engine_SRCS
    placesengine.cpp
    placeservice.cpp
    placesproxymodel.cpp
    setupdevicejob.cpp
    modeljob.h
)

set(CMAKE_AUTOMOC TRUE)

add_library(plasma_engine_places MODULE ${places_engine_SRCS})

target_link_libraries(plasma_engine_places
    KF5::Plasma
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::Solid
)

kservice_desktop_to_json(plasma_engine_places plasma-dataengine-places.desktop)

install(TARGETS plasma_engine_places
        DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-places.desktop
        DESTINATION ${SERVICES_INSTALL_DIR} )
install(FILES org.kde.places.operations
        DESTINATION ${DATA_INSTALL_DIR}/plasma/services )

