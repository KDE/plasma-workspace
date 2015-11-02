#ifndef KSCREENLOCKER_UNIT_TEST
#define KCHECKPASS_BIN "${CMAKE_INSTALL_FULL_LIBEXECDIR}/kcheckpass"
#else
#define KCHECKPASS_BIN "${CMAKE_CURRENT_BINARY_DIR}/greeter/autotests/fakekcheckpass"
#endif

#define KSCREENLOCKER_GREET_BIN "${CMAKE_INSTALL_FULL_LIBEXECDIR}/kscreenlocker_greet"
