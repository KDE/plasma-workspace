function(plasmoidtest plasmoidName)
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/testLayout.js.in
            ${CMAKE_CURRENT_BINARY_DIR}/testLayout-${plasmoidName}.js
            @ONLY
    )

    add_test(NAME ${plasmoidName}-test COMMAND
            plasmashell --test ${CMAKE_CURRENT_BINARY_DIR}/testLayout-plasmoidName.js
    )
endfunction()

plasmoidtest(org.kde.plasma.analogclock)
plasmoidtest(org.kde.plasma.kickoff)
