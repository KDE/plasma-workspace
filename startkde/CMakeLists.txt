add_subdirectory(kcminit)
add_subdirectory(kstartupconfig)

#FIXME: reconsider, looks fishy
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set(EXPORT_XCURSOR_PATH "XCURSOR_PATH=${CMAKE_INSTALL_PREFIX}/share/icons:$XCURSOR_PATH\":~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons\"; export XCURSOR_PATH")
endif()

configure_file(startkde.cmake ${CMAKE_CURRENT_BINARY_DIR}/startkde  @ONLY)

if(NOT WIN32)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startkde DESTINATION ${BIN_INSTALL_DIR})
endif()