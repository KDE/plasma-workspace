
########### next target ###############

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_calculatorrunner\")

find_package(Qalculate)
set_package_properties(Qalculate PROPERTIES DESCRIPTION "Qalculate Library"
                       URL "https://qalculate.github.io/"
                       TYPE OPTIONAL
                       PURPOSE "Needed to enable advanced features of the calculator runner"
                      )
if ( QALCULATE_FOUND )
    add_definitions(-DENABLE_QALCULATE)

    set(qalculate_engine_SRCS
    qalculate_engine.cpp
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    kde_enable_exceptions()
endif()

set(krunner_calculatorrunner_SRCS
    calculatorrunner.cpp
)

if ( QALCULATE_FOUND )
    add_library(krunner_calculatorrunner MODULE ${qalculate_engine_SRCS} ${krunner_calculatorrunner_SRCS})
    kcoreaddons_desktop_to_json(krunner_calculatorrunner plasma-runner-calculator.desktop )
    target_link_libraries(krunner_calculatorrunner
                          ${QALCULATE_LIBRARIES}
                          ${CLN_LIBRARIES}
                          KF5::KIOCore
                          KF5::Runner
                          KF5::I18n
                          Qt5::Network
                          Qt5::Widgets
    )
else ()
    add_library(krunner_calculatorrunner MODULE ${krunner_calculatorrunner_SRCS})
    kcoreaddons_desktop_to_json(krunner_calculatorrunner plasma-runner-calculator.desktop )
    target_link_libraries(krunner_calculatorrunner
                          KF5::Runner
                          KF5::I18n
                          Qt5::Gui
                          Qt5::Qml
    )
endif ()

install(TARGETS krunner_calculatorrunner DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )
