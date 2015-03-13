configure_file(${CMAKE_CURRENT_SOURCE_DIR}/metadata.desktop ${CMAKE_CURRENT_BINARY_DIR}/plasma-dataengine-share-addon-imgsusepasteorg.desktop COPYONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma-dataengine-share-addon-imgsusepasteorg.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES metadata.desktop
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/shareprovider/imgsusepasteorg)

install(DIRECTORY contents
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/shareprovider/imgsusepasteorg)
