include(ECMMarkAsTest)

find_package(Qt5Test CONFIG REQUIRED)

########### next target ###############

set(testclient_SRCS testclient.cpp)

add_executable(testclient ${testclient_SRCS})
add_test(testclient testclient)
ecm_mark_as_test(testclient)
target_link_libraries(testclient KF5XmlRpcClientPrivate Qt5::Test)

########### next target ###############

set(testquery_SRCS testquery.cpp)

add_executable(testquery ${testclient_SRCS})
add_test(testquery testquery)
ecm_mark_as_test(testquery)
target_link_libraries(testquery KF5XmlRpcClientPrivate Qt5::Test)
