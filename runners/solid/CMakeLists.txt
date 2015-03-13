add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_solid\")

set(krunner_solid_SRCS
    solidrunner.cpp
    devicewrapper.cpp
)

add_library(krunner_solid MODULE ${krunner_solid_SRCS})
target_link_libraries(krunner_solid  KF5::Plasma KF5::Solid KIOCore)

install(TARGETS krunner_solid DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES plasma-runner-solid.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
