set(KWIN_BIN "kwin_x11" CACHE STRING "Name of the KWin binary")

check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)     # ksmserver, ksplashml, sftp
check_include_files(unistd.h HAVE_UNISTD_H)

set(HAVE_FONTCONFIG ${FONTCONFIG_FOUND}) # kcms/{fonts,kfontinst}
