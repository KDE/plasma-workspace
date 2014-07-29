/* Define to 1 if you have the `_IceTransNoListen' function. */
#cmakedefine HAVE__ICETRANSNOLISTEN 1

#cmakedefine COMPILE_SCREEN_LOCKER 1

#ifndef KSMSERVER_UNIT_TEST
#define KCHECKPASS_BIN "${CMAKE_INSTALL_FULL_LIBEXECDIR}/kcheckpass"
#else
#define KCHECKPASS_BIN "${CMAKE_CURRENT_BINARY_DIR}/screenlocker/greeter/autotests/fakekcheckpass"
#endif

#define KSCREENLOCKER_GREET_BIN "${CMAKE_INSTALL_FULL_LIBEXECDIR}/kscreenlocker_greet"

#define KWIN_BIN "${KWIN_BIN}"