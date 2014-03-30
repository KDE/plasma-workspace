set(libklipper_common_SRCS
    klipper.cpp
    urlgrabber.cpp
    configdialog.cpp
    history.cpp
    historyitem.cpp
    historystringitem.cpp
    klipperpopup.cpp
    popupproxy.cpp
    historyimageitem.cpp
    historyurlitem.cpp
    actionstreewidget.cpp
    editactiondialog.cpp
    clipcommandprocess.cpp
)

# find_package(Prison QUIET CONFIG)
# set_package_properties(Prison PROPERTIES DESCRIPTION "Prison library"
#                        URL "http://projects.kde.org/prison"
#                        TYPE OPTIONAL
#                        PURPOSE "Needed to create mobile barcodes from clipboard data"
#                       )
# if (PRISON_FOUND)
#     add_definitions(-DHAVE_PRISON)
#     include_directories(${PRISON_INCLUDE_DIR})
# endif ()


kde4_add_app_icon(libklipper_common_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/klipper.png")
kde4_add_ui_files(libklipper_common_SRCS generalconfig.ui actionsconfig.ui editactiondialog.ui)
kconfig_add_kcfg_files(libklipper_common_SRCS klippersettings.kcfgc)

set(klipper_KDEINIT_SRCS ${libklipper_common_SRCS} main.cpp tray.cpp)


kf5_add_kdeinit_executable(klipper ${klipper_KDEINIT_SRCS})

set_target_properties(kdeinit_klipper PROPERTIES COMPILE_FLAGS -DHAVE_X11=1)
target_link_libraries(kdeinit_klipper Qt5::X11Extras KF5::GlobalAccel KF5::WindowSystem KF5::ConfigGui KF5::KDE4Support ${X11_LIBRARIES} ${ZLIB_LIBRARY})
if (X11_Xfixes_FOUND)
  target_link_libraries(kdeinit_klipper ${X11_Xfixes_LIB})
endif ()
# if (PRISON_FOUND)
#   target_link_libraries(kdeinit_klipper ${PRISON_LIBRARIES})
# endif ()

install(TARGETS kdeinit_klipper ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS klipper         ${INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS klipper.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(PROGRAMS klipper.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})

if(NOT WIN32)
  install( FILES klipper-kconfigxt.upd DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
endif()
