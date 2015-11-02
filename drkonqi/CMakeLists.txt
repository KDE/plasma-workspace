include (CheckFunctionExists)

check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("uname" HAVE_UNAME)

if (NOT DEBUG_PACKAGE_INSTALLER_NAME)
    set (DEBUG_PACKAGE_INSTALLER_NAME "installdbgsymbols.sh")
endif ()

configure_file (config-drkonqi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-drkonqi.h )

if (HAVE_X11)
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS X11Extras)
endif()

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1410)

add_subdirectory( data )
add_subdirectory( parser )
add_subdirectory( tests )
if ( WIN32 )
    add_subdirectory( kdbgwin )
endif ()

set(drkonqi_SRCS
    main.cpp
    drkonqidialog.cpp
    statuswidget.cpp
    aboutbugreportingdialog.cpp
    backtraceratingwidget.cpp
    backtracewidget.cpp
    backtracegenerator.cpp
    drkonqi.cpp
    drkonqibackends.cpp
    detachedprocessmonitor.cpp
    debugpackageinstaller.cpp
    systeminformation.cpp
    crashedapplication.cpp
    debugger.cpp
    debuggerlaunchers.cpp
    debuggermanager.cpp
    applicationdetailsexamples.cpp
    gdbhighlighter.cpp
)

ki18n_wrap_ui(drkonqi_SRCS
    ui/maindialog.ui
    ui/backtracewidget.ui
)

# if BACKTRACE_PARSER_DEBUG is enabled, it will show both the
# parsed and the unparsed backtrace in the backtrace widget.
# Comment this out for release.
#add_definitions(-DBACKTRACE_PARSER_DEBUG)


set(drkonqi_SRCS
    ${drkonqi_SRCS}
    bugzillaintegration/bugzillalib.cpp
    bugzillaintegration/reportassistantdialog.cpp
    bugzillaintegration/reportassistantpage.cpp
    bugzillaintegration/reportassistantpages_base.cpp
    bugzillaintegration/reportassistantpages_bugzilla.cpp
    bugzillaintegration/reportassistantpages_bugzilla_duplicates.cpp
    bugzillaintegration/reportinterface.cpp
    bugzillaintegration/productmapping.cpp
    bugzillaintegration/parsebugbacktraces.cpp # Requires kxmlrpcclient
    bugzillaintegration/duplicatefinderjob.cpp
)
ki18n_wrap_ui(drkonqi_SRCS
    bugzillaintegration/ui/assistantpage_introduction.ui
    bugzillaintegration/ui/assistantpage_bugawareness.ui
    bugzillaintegration/ui/assistantpage_conclusions.ui
    bugzillaintegration/ui/assistantpage_conclusions_dialog.ui
    bugzillaintegration/ui/assistantpage_bugzilla_login.ui
    bugzillaintegration/ui/assistantpage_bugzilla_duplicates.ui
    bugzillaintegration/ui/assistantpage_bugzilla_duplicates_dialog.ui
    bugzillaintegration/ui/assistantpage_bugzilla_duplicates_dialog_confirmation.ui
    bugzillaintegration/ui/assistantpage_bugzilla_information.ui
    bugzillaintegration/ui/assistantpage_bugzilla_preview.ui
    bugzillaintegration/ui/assistantpage_bugzilla_send.ui
)

add_executable(drkonqi ${drkonqi_SRCS})

target_compile_definitions(drkonqi PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(drkonqi
    KF5::I18n
    KF5::CoreAddons
    KF5::Service
    KF5::ConfigWidgets
    KF5::JobWidgets
    KF5::KIOCore
    KF5::Crash
    KF5::Completion
    Qt5::DBus

    KF5::XmlRpcClient
    KF5::WidgetsAddons
    KF5::Wallet
    drkonqi_backtrace_parser
)
if (HAVE_X11)
    target_link_libraries(drkonqi
        Qt5::X11Extras
    )
endif()

install(TARGETS drkonqi DESTINATION ${KDE_INSTALL_LIBEXECDIR})
