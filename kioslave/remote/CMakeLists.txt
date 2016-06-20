add_definitions(-DTRANSLATION_DOMAIN=\"kio_remote\") 

add_subdirectory( kdedmodule ) 
add_subdirectory( tests ) 

set(kio_remote_SRCS
    kio_remote.cpp
    remoteimpl.cpp
    )

ecm_qt_declare_logging_category(kio_remote_SRCS HEADER debug.h
                                               IDENTIFIER KIOREMOTE_LOG
                                               CATEGORY_NAME log_kioremote
                                               DEFAULT_SEVERITY Info)

add_library(kio_remote MODULE ${kio_remote_SRCS})
target_link_libraries(kio_remote  KF5::KIOCore KF5::KDELibs4Support)

install(TARGETS kio_remote  DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES remote.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
