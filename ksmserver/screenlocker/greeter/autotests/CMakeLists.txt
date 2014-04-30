add_definitions(-DKSMSERVER_UNIT_TEST)
include(ECMMarkAsTest)

#####################################
# fakekcheckpass
#####################################
add_executable(fakekcheckpass fakekcheckpass.c)
ecm_mark_nongui_executable(fakekcheckpass)
target_link_libraries(fakekcheckpass ${SOCKET_LIBRARIES})

#######################################
# AuthenticatorTest
#######################################
set( authenticatorTest_SRCS
     authenticatortest.cpp
     ../authenticator.cpp
)
add_executable(authenticatorTest ${authenticatorTest_SRCS})
target_link_libraries(authenticatorTest Qt5::Test)
add_test(ksmserver-authenticatorTest authenticatorTest)
ecm_mark_as_test(authenticatorTest)

#######################################
# KillTest
#######################################
add_executable(killTest killtest.cpp)
ecm_mark_as_test(killTest)
target_link_libraries(killTest Qt5::Test)
