find_package(Qt${QT_MAJOR_VERSION}QuickTest ${REQUIRED_QT_VERSION} CONFIG QUIET)

if(NOT TARGET Qt::QuickTest)
    message(STATUS "Qt5QuickTest not found, autotests will not be built.")
    return()
endif()

add_executable(qmltest qmltest.cpp)
target_link_libraries(qmltest Qt::QuickTest)

macro(qtquick_add_tests)
    if (WIN32)
        set(_extra_args -platform offscreen)
    endif()

    foreach(test ${ARGV})
        add_test(NAME ${test}
                 COMMAND qmltest
                        ${_extra_args}
                        -import ${CMAKE_BINARY_DIR}/bin
                        -input ${test}.qml
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endforeach()
endmacro()

qtquick_add_tests(
    tst_triangleFilter
)

include(ECMAddTests)

ecm_add_test(
    testrunnermodel.cpp
    LINK_LIBRARIES kickerplugin Qt::Test
)
