set(filebrowser_engine_SRCS
    filebrowserengine.cpp
)

add_library(plasma_engine_filebrowser MODULE ${filebrowser_engine_SRCS})
target_link_libraries(plasma_engine_filebrowser
    KF5::Plasma
    KF5::Service
    KF5::KIOCore
)

kcoreaddons_desktop_to_json(plasma_engine_filebrowser plasma-dataengine-filebrowser.desktop)

install(TARGETS plasma_engine_filebrowser DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-filebrowser.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

