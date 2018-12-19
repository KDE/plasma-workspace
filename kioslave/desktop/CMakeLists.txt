add_definitions(-DTRANSLATION_DOMAIN=\"kio_desktop\")

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

set(kio_desktop_SRCS kio_desktop.cpp)
qt5_add_dbus_interface( kio_desktop_SRCS ${KDED_DBUS_INTERFACE} kded_interface )

qt5_generate_dbus_interface( desktopnotifier.h ${CMAKE_CURRENT_BINARY_DIR}/desktopnotifier.xml )
qt5_add_dbus_interface( kio_desktop_SRCS ${CMAKE_CURRENT_BINARY_DIR}/desktopnotifier.xml desktopnotifier_interface )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(kio_desktop MODULE ${kio_desktop_SRCS})

target_link_libraries(kio_desktop Qt5::DBus Qt5::Gui KF5::KIOCore KF5::I18n Qt5::Network)

set_target_properties(kio_desktop PROPERTIES OUTPUT_NAME "desktop")

install(TARGETS kio_desktop DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### next target ###############

add_library(desktopnotifier MODULE desktopnotifier.cpp)
kcoreaddons_desktop_to_json(desktopnotifier desktopnotifier.desktop)
target_link_libraries(desktopnotifier KF5::KIOCore KF5::DBusAddons)

install(TARGETS desktopnotifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)

########### install files ###############

install( FILES desktop.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

install(FILES directory.desktop directory.trash DESTINATION ${KDE_INSTALL_DATADIR}/kio_desktop)
