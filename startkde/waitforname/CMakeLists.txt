
set(plasma_waitforname_SRCS
    waiter.cpp
    main.cpp
   )

ecm_qt_declare_logging_category(plasma_waitforname_SRCS HEADER debug_p.h IDENTIFIER LOG_PLASMA CATEGORY_NAME org.kde.knotifications)
ecm_mark_nongui_executable(ksyncdbusenv)

add_executable(plasma_waitforname ${plasma_waitforname_SRCS})

target_link_libraries(plasma_waitforname
    Qt5::DBus
   )

configure_file(org.freedesktop.Notifications.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Notifications.service)

install(TARGETS plasma_waitforname ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Notifications.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

