add_definitions(-DKSMSERVER_UNIT_TEST)
include(ECMMarkAsTest)

#######################################
# LogindTest
#######################################
set(logindTest_SRCS
    logindtest.cpp
    fakelogind.cpp
    ../logind.cpp
)
add_executable(logindTest ${logindTest_SRCS})
target_link_libraries(logindTest Qt5::DBus Qt5::Test)
add_test(ksmserver-logindTest logindTest)
ecm_mark_as_test(logindTest)
