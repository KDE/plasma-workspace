add_subdirectory(mockserver)

include(ECMAddTests)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

MACRO(PLASMASHELL_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
               set(test_SRCS
                   ${_testname}.cpp ../screenpool.cpp ../primaryoutputwatcher.cpp
                    )
               ecm_add_qtwayland_client_protocol(test_SRCS
                        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
                        BASENAME kde-primary-output-v1
                    )
               add_executable(${_testname} ${test_SRCS})
               target_link_libraries(${_testname}
                            Qt::Test
                            Qt::Gui
                            KF5::Service
                            KF5::WindowSystem
                            KF5::WaylandClient
                            Wayland::Client
                            Wayland::Server
                            SharedClientTest
                                     )
               if(HAVE_X11)
                  target_link_libraries(${_testname} XCB::XCB XCB::RANDR)
                  target_link_libraries(${_testname} Qt::X11Extras)
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
)
