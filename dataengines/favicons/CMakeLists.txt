set(favicons_engine_SRCS
    favicons.cpp
    faviconprovider.cpp
)

kcoreaddons_add_plugin(plasma_engine_favicons SOURCES ${favicons_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_link_libraries(plasma_engine_favicons
    KF5::Plasma
    KF5::KIOCore
    Qt::Gui
)

kcoreaddons_desktop_to_json(plasma_engine_favicons plasma-dataengine-favicons.desktop)
