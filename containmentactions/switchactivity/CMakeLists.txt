kcoreaddons_add_plugin(plasma_containmentactions_switchactivity SOURCES switch.cpp INSTALL_NAMESPACE "plasma/containmentactions")
kcoreaddons_desktop_to_json(plasma_containmentactions_switchactivity plasma-containmentactions-switchactivity.desktop)

target_link_libraries(plasma_containmentactions_switchactivity
                      Qt::Widgets
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::Activities
                      PW::KWorkspace)

install(FILES plasma-containmentactions-switchactivity.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
