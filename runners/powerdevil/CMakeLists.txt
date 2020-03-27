add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_powerdevil\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(krunner_powerdevil_SRCS
    PowerDevilRunner.cpp
)

add_library(krunner_powerdevil MODULE ${krunner_powerdevil_SRCS})
target_link_libraries(krunner_powerdevil Qt5::DBus KF5::ConfigCore KF5::I18n KF5::Plasma KF5::Runner KF5::KDELibs4Support)

install(TARGETS krunner_powerdevil DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES plasma-runner-powerdevil.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

