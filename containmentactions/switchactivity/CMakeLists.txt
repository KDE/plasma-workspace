kcoreaddons_add_plugin(plasma_containmentactions_switchactivity SOURCES switch.cpp switch.h INSTALL_NAMESPACE "plasma/containmentactions")

set_target_properties(plasma_containmentactions_switchactivity PROPERTIES
    OUTPUT_NAME switchactivity
)

target_link_libraries(plasma_containmentactions_switchactivity
                      Qt::Widgets
                      KF6::Plasma
                      KF6::KIOCore
                      Plasma::Activities
                      PW::KWorkspace)
