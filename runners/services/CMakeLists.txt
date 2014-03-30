set(krunner_services_SRCS
    servicerunner.cpp
)

add_library(krunner_services MODULE ${krunner_services_SRCS})
target_link_libraries(krunner_services
    KF5::KIOCore
    KF5::I18n
    #KF5::Plasma
    KF5::Runner
    KF5::KDE4Support
)

install(TARGETS krunner_services DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-services.desktop DESTINATION ${SERVICES_INSTALL_DIR})

