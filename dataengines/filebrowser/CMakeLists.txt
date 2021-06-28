set(filebrowser_engine_SRCS
    filebrowserengine.cpp
)

kcoreaddons_add_plugin(plasma_engine_filebrowser SOURCES ${filebrowser_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_link_libraries(plasma_engine_filebrowser
    KF5::Plasma
    KF5::Service
    KF5::KIOCore
)

kcoreaddons_desktop_to_json(plasma_engine_filebrowser plasma-dataengine-filebrowser.desktop)
