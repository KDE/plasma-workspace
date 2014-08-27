project(plasmainteractiveconsole)

add_definitions(-DTRANSLATION_DOMAIN=\"interactiveconsole\")

set(interactiveconsole_SRC
    interactiveconsole.cpp
    interactiveconsoleplugin.cpp
)

add_library(plasmainteractiveconsole SHARED ${interactiveconsole_SRC})
target_link_libraries(plasmainteractiveconsole
        Qt5::Quick
        Qt5::Script
        Qt5::Widgets
        KF5::Plasma
        KF5::I18n
        KF5::TextEditor)

install(TARGETS plasmainteractiveconsole DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/shell)

