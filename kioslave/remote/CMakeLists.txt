add_definitions(-DTRANSLATION_DOMAIN=\"kio_remote\") 

add_subdirectory( kdedmodule ) 
add_subdirectory( tests ) 

add_library(kio_remote MODULE kio_remote.cpp remoteimpl.cpp)
target_link_libraries(kio_remote  KF5::KIOCore KF5::KDELibs4Support)

install(TARGETS kio_remote  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES remote.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
