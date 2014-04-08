install(FILES crash.png DESTINATION ${DATA_INSTALL_DIR}/drkonqi/pics)
install(FILES mappings DESTINATION ${DATA_INSTALL_DIR}/drkonqi)
install(DIRECTORY debuggers DESTINATION ${DATA_INSTALL_DIR}/drkonqi PATTERN ".svn" EXCLUDE)
