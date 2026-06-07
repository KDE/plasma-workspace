# SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

if (BUILD_TESTING)
    find_library(LIBASAN NAMES asan)
    macro(add_timezone_selector_tests)
        foreach(test ${ARGV})
            add_test(NAME ${test}
                     COMMAND $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner>
                            -import ${CMAKE_BINARY_DIR}/bin
                            -input ${test}.qml
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            if(LIBASAN)
                set_tests_properties(${test} PROPERTIES ENVIRONMENT "LD_PRELOAD=${LIBASAN}")
            endif()
        endforeach()
    endmacro()

    add_timezone_selector_tests(
        tst_timezoneselector
    )
endif()