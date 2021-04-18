
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
    add_library(calculator MODULE ${qalculate_engine_SRCS} ${krunner_calculatorrunner_SRCS})
    kcoreaddons_desktop_to_json(calculator plasma-runner-calculator.desktop )
    target_link_libraries(calculator
                          ${QALCULATE_LIBRARIES}
                          ${CLN_LIBRARIES}
                          KF5::KIOCore
                          KF5::Runner
                          KF5::I18n
                          Qt::Network
                          Qt::Widgets
    )
else ()
    add_library(calculator MODULE ${krunner_calculatorrunner_SRCS})
    kcoreaddons_desktop_to_json(calculator plasma-runner-calculator.desktop )
    target_link_libraries(calculator
                          KF5::Runner
                          KF5::I18n
                          Qt::Gui
                          Qt::Qml
                          Qt::Widgets
    )
endif ()

set_target_properties(calculator PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/krunner")
install(TARGETS calculator DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
