set(krunner_recentdocuments_SRCS
    recentdocuments.cpp
)

add_library(krunner_recentdocuments MODULE ${krunner_recentdocuments_SRCS})

target_link_libraries(krunner_recentdocuments
    KF5::KIOCore
    KF5::I18n
    KF5::Runner
    KF5::KDE4Support
)

install(TARGETS krunner_recentdocuments DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES recentdocuments.desktop DESTINATION ${SERVICES_INSTALL_DIR})

