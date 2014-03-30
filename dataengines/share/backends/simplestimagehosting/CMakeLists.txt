configure_file(${CMAKE_CURRENT_SOURCE_DIR}/metadata.desktop ${CMAKE_CURRENT_BINARY_DIR}/plasma-dataengine-share-addon-simplestimagehosting.desktop COPYONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma-dataengine-share-addon-simplestimagehosting.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES metadata.desktop
        DESTINATION ${DATA_INSTALL_DIR}/plasma/shareprovider/simplestimagehosting/)

install(DIRECTORY contents
        DESTINATION ${DATA_INSTALL_DIR}/plasma/shareprovider/simplestimagehosting)
