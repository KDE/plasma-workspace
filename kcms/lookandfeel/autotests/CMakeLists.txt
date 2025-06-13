add_definitions(-DKSMSERVER_UNIT_TEST)
include(ECMMarkAsTest)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
add_executable(lookandfeelmanagertest lookandfeelmanagertest.cpp)
target_link_libraries(lookandfeelmanagertest lookandfeelmanager Qt::Test)

add_test(NAME lookandfeelmanagertest COMMAND lookandfeelmanagertest)
ecm_mark_as_test(lookandfeelmanagertest)
