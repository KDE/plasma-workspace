set(krunner_webshortcuts_SRCS
    webshortcutrunner.cpp
)

add_library(krunner_webshortcuts MODULE ${krunner_webshortcuts_SRCS})
target_link_libraries(krunner_webshortcuts KIOCore KF5::Plasma)

install(TARGETS krunner_webshortcuts DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-webshortcuts.desktop DESTINATION ${SERVICES_INSTALL_DIR})

