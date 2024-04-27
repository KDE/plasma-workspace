execute_process(COMMAND gcc -print-file-name=libasan.so OUTPUT_VARIABLE LIBASAN_PATH ECHO_OUTPUT_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
macro(qtquick_add_tests)
    foreach(test ${ARGV})
        add_test(NAME ${test}
                 COMMAND $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>
                        -import ${CMAKE_BINARY_DIR}/bin
                        -input ${test}.qml
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        set_tests_properties(${test} PROPERTIES ENVIRONMENT "LD_PRELOAD=${LIBASAN_PATH}")
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
