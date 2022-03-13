set(favicons_engine_SRCS
    favicons.cpp
    faviconprovider.cpp
    favicons.h
    faviconprovider.h
)

kcoreaddons_add_plugin(plasma_engine_favicons SOURCES ${favicons_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_link_libraries(plasma_engine_favicons
    KF5::Plasma
    KF5::KIOCore
    Qt::Gui
)
