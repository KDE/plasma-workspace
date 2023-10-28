add_executable(qmltest qmltest.cpp)
target_link_libraries(qmltest Qt::QuickTest)

macro(qtquick_add_tests)
    foreach(test ${ARGV})
        add_test(NAME ${test}
                 COMMAND qmltest
                        -import ${CMAKE_BINARY_DIR}/bin
                        -input ${test}.qml
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endforeach()
endmacro()

qtquick_add_tests(
    tst_triangleFilter
)

ecm_add_test(
    testrunnermodel.cpp
    LINK_LIBRARIES kickerplugin Qt::Test
)
set_tests_properties(testrunnermodel PROPERTIES REPEAT UNTIL_PASS:2)
