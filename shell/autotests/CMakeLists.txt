add_subdirectory(mockserver)

include(ECMAddTests)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

MACRO(PLASMASHELL_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
                ecm_qt_declare_logging_category(${_testname} HEADER debug.h
                                               IDENTIFIER PLASMASHELL
                                               CATEGORY_NAME kde.plasmashell
                                               DEFAULT_SEVERITY Info)
                set(USE_SCRIPTING 0)
                configure_file(../config-plasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma.h)
                set(test_SRCS
                    ${_testname}.cpp
                    debug.cpp
                    ../alternativeshelper.cpp
                    ../screenpool.cpp
                    ../shellcorona.cpp
                    ../strutmanager.cpp
                    ../shellcontainmentconfig.cpp
                    ../osd.cpp
                    ../panelview.cpp
                    ../panelconfigview.cpp
                    ../containmentconfigview.cpp
                    ../currentcontainmentactionsmodel.cpp
                    ../panelshadows.cpp
                    ../desktopview.cpp
                    ${CMAKE_CURRENT_BINARY_DIR}/../screenpool-debug.cpp ../outputorderwatcher.cpp
                        )
               set(plasmashell_dbusXML ../dbus/org.kde.PlasmaShell.xml)
               qt_add_dbus_adaptor(test_SRCS ${plasmashell_dbusXML} shellcorona.h ShellCorona plasmashelladaptor)
               set(krunner_xml ../../krunner/dbus/org.kde.krunner.App.xml)
               qt_add_dbus_interface(test_SRCS ${krunner_xml} krunner_interface)

               include_directories(${CMAKE_CURRENT_BINARY_DIR}/../mockserver)
               add_executable(${_testname} ${test_SRCS})
               target_link_libraries(${_testname}
                            Qt::Test
                            Qt::Gui
                            Qt::WaylandClient
                            KF5::Activities
                            KF5::ConfigCore
                            KF5::Declarative
                            KF5::Service
                            KF5::WaylandClient
                            KF5::WindowSystem
                            KF5::Package
                            KF5::Plasma
                            KF5::PlasmaQuick
                            KF5::QuickAddons
                            KF5::XmlGui
                            KF5::I18n
                            KF5::GlobalAccel
                            Qt::Quick
                            Qt::DBus
                            Wayland::Client
                            Wayland::Server
                            SharedClientTest
                                     )
               if (QT_MAJOR_VERSION STREQUAL "6")
                    target_link_libraries(${_testname} KF5::ConfigQml)
               endif()
               if(HAVE_X11)
                  target_link_libraries(${_testname} XCB::XCB XCB::RANDR)
                  if (QT_MAJOR_VERSION EQUAL "5")
                    target_link_libraries(${_testname} Qt::X11Extras)
                  else()
                    target_link_libraries(${_testname} Qt::GuiPrivate)
                  endif()
               endif()
               if(QT_QTOPENGL_FOUND)
                   target_link_libraries(${_testname} Qt::OpenGL)
               endif()
               add_test(NAME ${_testname} COMMAND ${_testname})
               ecm_mark_as_test(${_testname})
       ENDFOREACH(_testname)
ENDMACRO(PLASMASHELL_UNIT_TESTS)

PLASMASHELL_UNIT_TESTS(
    screenpooltest
    shelltest
)
