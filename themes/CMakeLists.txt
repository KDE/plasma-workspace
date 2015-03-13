
set( themes_RC
    qtcde.themerc
    qtcleanlooks.themerc
    qtmotif.themerc
    qtplastique.themerc
    qtwindows.themerc
)

if (X11_FOUND)
  set( themes_RC ${themes_RC}
      qtgtk.themerc
  )
endif ()

if (WIN32)
  set( themes_RC ${themes_RC}
      qtwindowsvista.themerc
      qtwindowsxp.themerc
  )
endif ()

if (APPLE)
  set( themes_RC ${themes_RC}
      qtmacintosh.themerc
  )
endif ()


########### install files ###############

install( FILES ${themes_RC}  DESTINATION  ${KDE_INSTALL_DATADIR}/kstyle/themes )




