set(switchactivity_SRCS
    switch.cpp
)

add_library(plasma_containmentactions_switchactivity MODULE ${switchactivity_SRCS})

kcoreaddons_desktop_to_json(plasma_containmentactions_switchactivity plasma-containmentactions-switchactivity.desktop)

target_link_libraries(plasma_containmentactions_switchactivity
                      Qt5::Widgets
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::Activities
                      PW::KWorkspace)

install(TARGETS plasma_containmentactions_switchactivity DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-containmentactions-switchactivity.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
