function(plasmoidtest plasmoidName)
    if (Qt5Core_VERSION_STRING VERSION_LESS "5.6.1")
        return()
    endif ()
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/testLayout.js.in
            ${CMAKE_CURRENT_BINARY_DIR}/testLayout-${plasmoidName}.js
            @ONLY
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.sh.in
                   ${CMAKE_CURRENT_BINARY_DIR}/test-${plasmoidName}.sh
                   @ONLY
    )

    add_test(NAME ${plasmoidName}-test COMMAND
            sh ${CMAKE_CURRENT_BINARY_DIR}/test-${plasmoidName}.sh
    )
endfunction()

plasmoidtest(org.kde.plasma.analogclock)
plasmoidtest(org.kde.plasma.kickoff)
