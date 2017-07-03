add_subdirectory(crashtest)
add_subdirectory(backtraceparsertest)
if(KF5XmlRpcClient_FOUND)
    add_subdirectory(bugzillalibtest)
endif()

if(NOT RUBY_EXECTUABLE)
    find_program(RUBY_EXECUTABLE ruby)
endif()
if(RUBY_EXECUTABLE)
    execute_process(COMMAND ${RUBY_EXECUTABLE} -e "require 'atspi'"
        RESULT_VARIABLE RUBY_ATSPI)
    execute_process(COMMAND ${RUBY_EXECUTABLE} -e "require 'xmlrpc/server'"
        RESULT_VARIABLE RUBY_XMLRPC)
endif()
if(NOT XVFB_RUN_EXECTUABLE)
    find_program(XVFB_RUN_EXECTUABLE xvfb-run)
endif()
if(NOT ATSPI_BUS_LAUNCHER_EXECUTABLE)
    find_program(ATSPI_BUS_LAUNCHER_EXECUTABLE
        NAMES at-spi-bus-launcher
        PATHS
            /usr/lib/at-spi2-core/ # debians
            /usr/lib/at-spi2/      # suses
        DOC "AT-SPI accessibility dbus launcher")
endif()

if(RUBY_EXECUTABLE AND XVFB_RUN_EXECTUABLE AND ATSPI_BUS_LAUNCHER_EXECUTABLE
   AND RUBY_ATSPI EQUAL 0 AND RUBY_XMLRPC EQUAL 0)
    set(WITH_DRKONI_INTEGRATION_TESTING TRUE)
endif()
add_feature_info(DrKonqiIntegrationTesting WITH_DRKONI_INTEGRATION_TESTING
    "Needs Ruby, functional atspi and xmlrpc gems, as well as xvfb-run.")
add_subdirectory(integration)
