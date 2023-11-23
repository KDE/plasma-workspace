set(filebrowser_engine_SRCS
    filebrowserengine.cpp
    filebrowserengine.h
)

kcoreaddons_add_plugin(plasma_engine_filebrowser SOURCES ${filebrowser_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_link_libraries(plasma_engine_filebrowser
    Plasma::Plasma
    KF6::Plasma5Support
    KF6::Service
    KF6::KIOCore
)
