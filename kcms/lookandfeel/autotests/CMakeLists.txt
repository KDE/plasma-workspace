add_definitions(-DKSMSERVER_UNIT_TEST)
include(ECMMarkAsTest)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
add_executable(kcmTest kcmtest.cpp)
target_link_libraries(kcmTest kcm_lookandfeel_objects Qt::Test)

add_test(NAME lookandfeel-kcmTest COMMAND kcmTest)
ecm_mark_as_test(kcmTest)

