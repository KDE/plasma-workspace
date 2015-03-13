set(favicons_engine_SRCS
    favicons.cpp
    faviconprovider.cpp
)

add_library(plasma_engine_favicons MODULE ${favicons_engine_SRCS} )
target_link_libraries(plasma_engine_favicons
    KF5::Plasma
    KF5::KIOCore
    Qt5::Gui
)

kcoreaddons_desktop_to_json(plasma_engine_favicons plasma-dataengine-favicons.desktop)

install(TARGETS plasma_engine_favicons DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine )
install(FILES plasma-dataengine-favicons.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )


