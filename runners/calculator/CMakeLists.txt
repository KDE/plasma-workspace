
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
    kcoreaddons_add_plugin(calculator SOURCES ${qalculate_engine_SRCS} ${krunner_calculatorrunner_SRCS} INSTALL_NAMESPACE "kf5/krunner")
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
    kcoreaddons_add_plugin(calculator SOURCES ${krunner_calculatorrunner_SRCS} INSTALL_NAMESPACE "kf5/krunner")
    kcoreaddons_desktop_to_json(calculator plasma-runner-calculator.desktop )
    target_link_libraries(calculator
                          KF5::Runner
                          KF5::I18n
                          Qt::Gui
                          Qt::Qml
                          Qt::Widgets
    )
endif ()

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
