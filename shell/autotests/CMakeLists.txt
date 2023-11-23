add_subdirectory(mockserver)

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
		    ../autohidescreenedge.cpp
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
                            Plasma::Activities
                            KF6::ConfigCore
                            KF6::Service
                            Plasma::KWaylandClient
                            KF6::WindowSystem
                            KF6::Package
                            KF6::Plasma
                            KF6::PlasmaQuick
                            KF6::Svg
                            KF6::XmlGui
                            KF6::I18n
                            KF6::GlobalAccel
                            Qt::Quick
                            Qt::DBus
                            Wayland::Client
                            Wayland::Server
                            LayerShellQt::Interface
                            SharedClientTest
                                     )
               target_link_libraries(${_testname} KF6::ConfigQml)
               if(HAVE_X11)
                  target_link_libraries(${_testname} XCB::XCB XCB::RANDR)
                  target_link_libraries(${_testname} Qt::GuiPrivate)
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

