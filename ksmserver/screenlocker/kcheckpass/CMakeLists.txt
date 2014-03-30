project(KCheckPass)

include_directories( ${UNIXAUTH_INCLUDE_DIRS} )
check_include_files(paths.h HAVE_PATHS_H)
configure_file (config-kcheckpass.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcheckpass.h )

set(kcheckpass_SRCS
	kcheckpass.h
	kcheckpass.c
	checkpass_etcpasswd.c
	checkpass_pam.c
	checkpass_shadow.c
	checkpass_osfc2passwd.c
	checkpass_aix.c
)

add_executable(kcheckpass ${kcheckpass_SRCS})
ecm_mark_nongui_executable(kcheckpass)

if (KDE4_ENABLE_FPIE)
    set_property(TARGET kcheckpass APPEND_STRING PROPERTY COMPILE_FLAGS " ${KDE4_CXX_FPIE_FLAGS}")
    set_property(TARGET kcheckpass APPEND_STRING PROPERTY LINK_FLAGS " ${KDE4_PIE_LDFLAGS}")
endif ()

set_property(TARGET kcheckpass APPEND_STRING PROPERTY COMPILE_FLAGS " -U_REENTRANT")
target_link_libraries(kcheckpass ${UNIXAUTH_LIBRARIES} ${SOCKET_LIBRARIES})
install(TARGETS kcheckpass DESTINATION ${LIBEXEC_INSTALL_DIR})
install(CODE "
    set(KCP_PATH \"\$ENV{DESTDIR}${LIBEXEC_INSTALL_DIR}/kcheckpass\")
    execute_process(COMMAND sh -c \"chown root '\${KCP_PATH}' && chmod +s '\${KCP_PATH}'\")
")

#EXTRA_DIST = README
