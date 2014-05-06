
set(krunner_placesrunner_SRCS
    placesrunner.cpp
)

add_library(krunner_placesrunner MODULE ${krunner_placesrunner_SRCS})

target_link_libraries(krunner_placesrunner
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::KIOFileWidgets
    KF5::I18n
    KF5::Runner
)

install(TARGETS krunner_placesrunner DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-places.desktop DESTINATION ${SERVICES_INSTALL_DIR})

