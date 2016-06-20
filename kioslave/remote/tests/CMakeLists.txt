set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
########### next target ###############
set(testremote_SRCS testremote.cpp ${kio_remote_SRCS} )

add_executable(testremote ${testremote_SRCS})
ecm_mark_as_test(testremote)

target_link_libraries(testremote KF5::KDELibs4Support KF5::KIOCore )





