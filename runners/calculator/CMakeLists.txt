
########### next target ###############

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_calculatorrunner\")

set(EXTERNAL_LIBS
    KF5::KIOCore
    KF5::Runner
    KF5::I18n
    Qt::Network
    Qt::Widgets
)

set(qalculate_engine_SRCS
    qalculate_engine.cpp
)
set(krunner_calculatorrunner_SRCS
    calculatorrunner.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
kde_enable_exceptions()

kcoreaddons_add_plugin(calculator SOURCES ${qalculate_engine_SRCS} ${krunner_calculatorrunner_SRCS} INSTALL_NAMESPACE "kf5/krunner")
target_link_libraries(calculator
                      ${QALCULATE_LIBRARIES}
                      ${CLN_LIBRARIES}
                      ${EXTERNAL_LIBS}
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
