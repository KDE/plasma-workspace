add_definitions(-DTRANSLATION_DOMAIN=\"kcm_location\")
kcmutils_add_qml_kcm(kcm_location SOURCES
    kcm.cpp
    settings.cpp
)
target_link_libraries(kcm_location PRIVATE
    Qt6::Positioning
    Qt6::Quick
    KF6::AuthCore
    KF6::ConfigGui
    KF6::KCMUtilsQuick
)

add_executable(kcm_location_helper)
target_sources(kcm_location_helper PRIVATE
    helper.cpp
)
target_link_libraries(kcm_location_helper PRIVATE
    Qt6::Core
    KF6::AuthCore
    KF6::ConfigCore
)
install(TARGETS kcm_location_helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(kcm_location_helper org.kde.kcm.location root)
kauth_install_actions(org.kde.kcm.location kcm_location.actions)
