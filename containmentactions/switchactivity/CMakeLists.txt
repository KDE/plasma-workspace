kcoreaddons_add_plugin(plasma_containmentactions_switchactivity SOURCES switch.cpp switch.h INSTALL_NAMESPACE "plasma/containmentactions")

target_link_libraries(plasma_containmentactions_switchactivity
                      Qt::Widgets
                      KF6::Plasma
                      KF6::KIOCore
                      KF6::Activities
                      PW::KWorkspace)
