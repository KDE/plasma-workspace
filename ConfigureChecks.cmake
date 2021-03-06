set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )

set(KWIN_BIN "kwin_x11" CACHE STRING "Name of the KWin binary")

find_program(some_x_program NAMES iceauth xrdb xterm)
if (NOT some_x_program)
    set(some_x_program /usr/bin/xrdb)
    message("Warning: Could not determine X binary directory. Assuming /usr/bin.")
endif (NOT some_x_program)
get_filename_component(proto_xbindir "${some_x_program}" PATH)

check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)     # ksmserver, ksplashml, sftp
check_include_files(unistd.h HAVE_UNISTD_H)

set(HAVE_FONTCONFIG ${FONTCONFIG_FOUND}) # kcms/{fonts,kfontinst}
set(HAVE_XCURSOR ${X11_Xcursor_FOUND}) # many uses
set(HAVE_XFIXES ${X11_Xfixes_FOUND}) # klipper, kicker
