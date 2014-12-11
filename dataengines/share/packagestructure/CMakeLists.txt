set(sharepackage_SRCS
        share_package.cpp
)

add_library(plasma_packagestructure_share MODULE ${sharepackage_SRCS})
kcoreaddons_desktop_to_json(plasma_packagestructure_share plasma-packagestructure-share.desktop)
target_link_libraries(plasma_packagestructure_share
                      KF5::Plasma
                      KF5::I18n
                      KF5::Service
                      Qt5::Gui
)

install(TARGETS plasma_packagestructure_share
        DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/packagestructure)
#install(FILES data/plasma-packagestructure-share.desktop DESTINATION ${SERVICES_INSTALL_DIR})


