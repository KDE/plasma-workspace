set(interactiveconsole_SRCS
    interactiveconsole.cpp
    main.cpp
)

add_executable(plasma-interactiveconsole ${interactiveconsole_SRCS})
target_link_libraries(plasma-interactiveconsole Qt::Widgets Qt::DBus KF6::KIOCore KF6::WidgetsAddons KF6::ConfigWidgets KF6::WindowSystem KF6::TextEditor KF6::Package)
install(TARGETS plasma-interactiveconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
