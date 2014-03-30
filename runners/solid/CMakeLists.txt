set(krunner_solid_SRCS
    solidrunner.cpp
    devicewrapper.cpp
)

add_library(krunner_solid MODULE ${krunner_solid_SRCS})
target_link_libraries(krunner_solid ${KDE4_KDEUI_LIBS} KF5::Plasma KF5::Solid KIOCore)

install(TARGETS krunner_solid DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-solid.desktop DESTINATION ${SERVICES_INSTALL_DIR})
