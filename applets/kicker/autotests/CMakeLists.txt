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
    ../runnermodel.cpp
    ../runnermatchesmodel.cpp
    ../abstractmodel.cpp
    ../actionlist.cpp
    ../containmentinterface.cpp
    ../processrunner.cpp
    TEST_NAME testrunnermodel
    LINK_LIBRARIES Qt::Test Qt::Quick KF6::ConfigCore KF6::Runner KF6::Service KF6::I18n KF6::KIOGui KF6::KIOWidgets KF6::JobWidgets Plasma::Plasma Plasma::PlasmaQuick Plasma::Activities Plasma::ActivitiesStats
)

if (${HAVE_APPSTREAMQT})
target_link_libraries(testrunnermodel AppStreamQt)
endif()

set_tests_properties(testrunnermodel PROPERTIES REPEAT UNTIL_PASS:2)
