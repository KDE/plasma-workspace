add_test(NAME drkonqi_integration_suite
    COMMAND ${RUBY_EXECTUABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/suite
        --drkonqi $<TARGET_FILE:drkonqi>
        --at-spi-bus-launcher ${ATSPI_BUS_LAUNCHER_EXECUTABLE})

# Hack to get rb files to show in qtc.
file(GLOB RUBIES suite *.rb)
add_custom_target(Rubies ALL echo SOURCES ${RUBIES})
