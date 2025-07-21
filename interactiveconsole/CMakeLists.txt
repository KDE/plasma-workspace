add_definitions(-DTRANSLATION_DOMAIN=\"plasma_interactiveconsole\")

set(interactiveconsole_SRCS
    interactiveconsole.cpp interactiveconsole.h
    main.cpp
)

add_executable(plasma-interactiveconsole ${interactiveconsole_SRCS})
target_link_libraries(plasma-interactiveconsole Qt::Widgets Qt::DBus KF6::I18n KF6::KIOCore KF6::WidgetsAddons KF6::TextEditor KF6::Package KF6::TextWidgets)
install(TARGETS plasma-interactiveconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.plasma-interactiveconsole.desktop DESTINATION ${KDE_INSTALL_APPDIR})
