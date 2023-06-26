set(kde_PART_SRCS
    debug.cpp
    kdeplatformplugin.cpp
    kiomediastream.cpp
)

kcoreaddons_add_plugin(kde SOURCES ${kde_PART_SRCS} INSTALL_NAMESPACE "phonon_platform")
target_link_libraries(kde Phonon::phonon4qt6 KF6::CoreAddons KF6::ConfigCore KF6::WidgetsAddons KF6::I18n KF6::KIOCore KF6::Notifications)

install(FILES phonon.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
