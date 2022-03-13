kcoreaddons_add_plugin(plasma_containmentactions_switchactivity SOURCES switch.cpp switch.h INSTALL_NAMESPACE "plasma/containmentactions")

target_link_libraries(plasma_containmentactions_switchactivity
                      Qt::Widgets
                      KF5::Plasma
                      KF5::KIOCore
                      KF5::Activities
                      PW::KWorkspace)
