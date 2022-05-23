add_subdirectory(mockserver)

include(ECMAddTests)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

MACRO(PLASMASHELL_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
                set(test_SRCS
                    ${_testname}.cpp ../screenpool.cpp ${CMAKE_CURRENT_BINARY_DIR}/../screenpool-debug.cpp ../primaryoutputwatcher.cpp
                        )
               include_directories(${CMAKE_CURRENT_BINARY_DIR}/../mockserver ${Libdrm_INCLUDE_DIR})
               add_executable(${_testname} ${test_SRCS})
               target_link_libraries(${_testname}
                            Qt::Test
                            Qt::Gui
                            KF5::ConfigCore
                            KF5::Service
                            KF5::WaylandClient
                            KF5::WindowSystem
                            Wayland::Client
                            Wayland::Server
                            Libdrm::Libdrm
                            SharedClientTest
                                     )
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
)
