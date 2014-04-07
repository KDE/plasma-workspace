set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(libkioremote_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../kio_remote.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../remoteimpl.cpp )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
########### next target ###############
set(testremote_SRCS testremote.cpp ${libkioremote_SRCS} )

add_executable(testremote ${testremote_SRCS})
ecm_mark_as_test(testremote)

target_link_libraries(testremote KF5::KDE4Support KF5::KIOCore )





