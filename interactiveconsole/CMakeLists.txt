set(interactiveconsole_SRCS
    interactiveconsole.cpp
    main.cpp
)

add_executable(plasma-interactiveconsole ${interactiveconsole_SRCS})
target_link_libraries(plasma-interactiveconsole Qt::Widgets Qt::DBus KF5::KIOCore KF5::WidgetsAddons KF5::ConfigWidgets KF5::WindowSystem KF5::TextEditor KF5::Package)
install(TARGETS plasma-interactiveconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
