
set(plasma_waitforname_SRCS
    waiter.cpp
    main.cpp
   )

ecm_qt_declare_logging_category(plasma_waitforname_SRCS HEADER debug_p.h IDENTIFIER LOG_PLASMA CATEGORY_NAME org.kde.knotifications)

add_executable(plasma_waitforname ${plasma_waitforname_SRCS})
ecm_mark_nongui_executable(plasma_waitforname)

target_link_libraries(plasma_waitforname
    Qt5::DBus
   )

configure_file(org.kde.plasma.Notifications.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasma.Notifications.service)

configure_file(org.kde.KSplash.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KSplash.service)

install(TARGETS plasma_waitforname ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasma.Notifications.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KSplash.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
