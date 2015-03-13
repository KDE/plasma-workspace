install(FILES crash.png DESTINATION ${KDE_INSTALL_DATADIR}/drkonqi/pics)
install(FILES mappings DESTINATION ${KDE_INSTALL_DATADIR}/drkonqi)
install(DIRECTORY debuggers DESTINATION ${KDE_INSTALL_DATADIR}/drkonqi PATTERN ".svn" EXCLUDE)
