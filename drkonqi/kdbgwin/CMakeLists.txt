set(kdbgwin_SRCS
    common.h
    main.cpp
    process.h
    process.cpp
    abstract_generator.h
    abstract_generator.cpp
    callbacks.h
    callbacks.cpp
    outputters.h
    outputters.cpp
)

set(COMMON_LIBS dbghelp psapi shlwapi)

if ( MINGW )
    # FIXME this should be handled in a higher level cmake with proper find
    # scripts and configuration
    add_executable(kdbgwin ${kdbgwin_SRCS} mingw_generator.h mingw_generator.cpp)
    target_link_libraries(kdbgwin ${COMMON_LIBS} bfd iberty intl z)
endif ()
if ( MSVC OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
    add_executable(kdbgwin ${kdbgwin_SRCS} msvc_generator.h msvc_generator.cpp)
    target_link_libraries(kdbgwin ${COMMON_LIBS})
endif ()

install(TARGETS kdbgwin DESTINATION ${LIBEXEC_INSTALL_DIR})
