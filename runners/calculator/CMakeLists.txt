
########### next target ###############

#macro_optional_find_package(Qalculate)
#set_package_properties(Qalculate PROPERTIES DESCRIPTION "Qalculate Library"
#                       URL "http://qalculate.sourceforge.net"
#                       TYPE OPTIONAL
#                       PURPOSE "Needed to enable advanced features of the calculator runner"
#                      )
#if ( QALCULATE_FOUND )
#    add_definitions(-DENABLE_QALCULATE)
#
#    set(qalculate_engine_SRCS
#    qalculate_engine.cpp
#    )
#
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -Wno-deprecated")
#endif()

set(krunner_calculatorrunner_SRCS
    calculatorrunner.cpp
)

#if ( QALCULATE_FOUND )
#    add_library(krunner_calculatorrunner MODULE ${qalculate_engine_SRCS} ${krunner_calculatorrunner_SRCS})
#    target_link_libraries(krunner_calculatorrunner ${QALCULATE_LIBRARIES} ${CLN_LIBRARIES} KF5::Solid KIOCore KF5::Runner KF5::KDELibs4Support KF5::Plasma )
#else ()
    add_library(krunner_calculatorrunner MODULE ${krunner_calculatorrunner_SRCS})
    target_link_libraries(krunner_calculatorrunner
                          KF5::Plasma
                          KF5::Runner
                          KF5::I18n
                          KF5::KDELibs4Support
                          Qt5::Script
    )
#endif ()

install(TARGETS krunner_calculatorrunner DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install(FILES plasma-runner-calculator.desktop DESTINATION ${SERVICES_INSTALL_DIR})
