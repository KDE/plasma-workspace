set(sharepackage_SRCS
        share_package.cpp
)

add_library(plasma_packagestructure_share MODULE ${sharepackage_SRCS})
kcoreaddons_desktop_to_json(plasma_packagestructure_share plasma-packagestructure-share.desktop)
target_link_libraries(plasma_packagestructure_share
                      KF5::Package
                      KF5::I18n
                      KF5::Service
                      Qt5::Gui
)

install(TARGETS plasma_packagestructure_share
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/packagestructure)
#install(FILES data/plasma-packagestructure-share.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})


