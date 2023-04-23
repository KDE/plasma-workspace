# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_calculatorrunner\")

set(qalculate_engine_SRCS
    qalculate_engine.cpp
    qalculate_engine.h
)
set(krunner_calculatorrunner_SRCS
    calculatorrunner.cpp
    calculatorrunner.h
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
kde_enable_exceptions()

kcoreaddons_add_plugin(calculator SOURCES ${qalculate_engine_SRCS} ${krunner_calculatorrunner_SRCS} INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(calculator
    PkgConfig::QALCULATE
    KF6::KIOCore
    KF6::Runner
    KF6::I18n
    Qt::Network
    Qt::Widgets
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
