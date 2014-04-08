add_executable( backtraceparsertest fakebacktracegenerator.cpp backtraceparsertest.cpp)
target_link_libraries( backtraceparsertest Qt5::Test Qt5::Core KF5::KIOWidgets drkonqi_backtrace_parser)
ecm_mark_as_test(backtraceparsertest)
add_test(backtraceparsertest backtraceparsertest)

add_executable(backtraceparsertest_manual fakebacktracegenerator.cpp backtraceparsertest_manual.cpp)
target_link_libraries(backtraceparsertest_manual Qt5::Core KF5::I18n KF5::KIOWidgets drkonqi_backtrace_parser)
ecm_mark_as_test(backtraceparsertest_manual)

# create a link so that the data directory can be accessed from the build directory, where the test runs
if( NOT WIN32 )
	if ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
		add_custom_command(TARGET backtraceparsertest POST_BUILD
						   COMMAND ${CMAKE_COMMAND} -E create_symlink
								${CMAKE_CURRENT_SOURCE_DIR}/backtraceparsertest_data backtraceparsertest_data
						   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
						  )
	endif ()
endif()
