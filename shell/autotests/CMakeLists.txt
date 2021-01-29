include(ECMAddTests)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

MACRO(PLASMASHELL_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
               add_executable(${_testname} ${_testname}.cpp ../screenpool.cpp )
               target_link_libraries(${_testname}
                            Qt::Test
                            Qt::Gui
                            KF5::Service
                            KF5::WindowSystem
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
