project(drkonqi)

include (CheckFunctionExists)

check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("uname" HAVE_UNAME)

if (NOT DEBUG_PACKAGE_INSTALLER_NAME)
    set (DEBUG_PACKAGE_INSTALLER_NAME "installdbgsymbols.sh")
endif ()

configure_file (config-drkonqi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-drkonqi.h )

include_directories(${KDEPIMLIBS_INCLUDE_DIRS})

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

## TODO Remove internal copy when kxmlrpcclient is released
set (KF5XmlRpcClient_FOUND FALSE)
if(NOT KF5XmlRpcClient_FOUND)
    add_subdirectory ( internalkxmlrpcclient )
    include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/internalkxmlrpcclient/src )
    target_link_libraries ( drkonqi KF5XmlRpcClientPrivate
    KF5::WebKit
    KF5::WidgetsAddons
    KF5::Completion
    KF5::Wallet
)
endif()

target_link_libraries(drkonqi
    KF5::I18n
    KF5::WindowSystem
    KF5::CoreAddons
    KF5::Service
    KF5::ConfigWidgets
    KF5::JobWidgets
    KF5::KIOCore
    KF5::Crash
    Qt5::DBus
    drkonqi_backtrace_parser
)

install(TARGETS drkonqi DESTINATION ${LIBEXEC_INSTALL_DIR})
