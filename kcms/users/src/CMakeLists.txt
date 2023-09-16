set_source_files_properties(org.freedesktop.Accounts.xml PROPERTIES NO_NAMESPACE TRUE)
set_source_files_properties(org.freedesktop.Accounts.User.xml PROPERTIES NO_NAMESPACE TRUE)
set_source_files_properties(net.reactivated.Fprint.Device.xml PROPERTIES NO_NAMESPACE TRUE)
set_source_files_properties(net.reactivated.Fprint.Manager.xml PROPERTIES NO_NAMESPACE TRUE)
                        
qt_add_dbus_interface(kcm_users_DBUS_SRCS
    org.freedesktop.Accounts.xml
    accounts_interface
)

qt_add_dbus_interface(kcm_users_DBUS_SRCS
    org.freedesktop.Accounts.User.xml
    user_interface
)

qt_add_dbus_interface(kcm_users_DBUS_SRCS
    net.reactivated.Fprint.Device.xml
    fprint_device_interface
)

qt_add_dbus_interface(kcm_users_DBUS_SRCS
    net.reactivated.Fprint.Manager.xml
    fprint_manager_interface
)

set(login1_manager_xml org.freedesktop.login1.Manager.xml)
set_source_files_properties(${login1_manager_xml} PROPERTIES INCLUDE "usersessions.h")
qt_add_dbus_interface(kcm_users_DBUS_SRCS
    ${login1_manager_xml}
    login1_interface
)

kcmutils_add_qml_kcm(kcm_users)
target_sources(kcm_users PRIVATE
    kcm.cpp
    maskmousearea.cpp
    user.cpp
    usermodel.cpp
    fprintdevice.cpp
    fingerprintmodel.cpp
    kcm.h
    user.h
    usermodel.h
    fprintdevice.h
    fingerprintmodel.h
    ${kcm_users_DBUS_SRCS}
)

ecm_qt_declare_logging_category(kcm_users
    HEADER kcmusers_debug.h
    IDENTIFIER KCMUSERS
    CATEGORY_NAME org.kde.kcm_users
    EXPORT KCMUSERS
    DESCRIPTION "System Settings - Users"
)
ecm_qt_install_logging_categories(EXPORT KCMUSERS DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

target_link_libraries(kcm_users PRIVATE
    Qt::Quick
    Qt::QuickWidgets
    Qt::Concurrent
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtilsQuick
    KF6::Wallet
    Qt::DBus
    crypt
)
