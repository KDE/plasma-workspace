set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include(ECMMarkAsTest)

########### next target ###############

set(kuiservertest_SRCS kuiservertest.cpp)


add_executable(kuiservertest ${kuiservertest_SRCS})

target_link_libraries(kuiservertest KF5::KIOCore KF5::I18n KF5::KIOWidgets)

ecm_mark_as_test(kuiservertest)
