set(activity_engine_SRCS
        ActivityData.cpp
        activityengine.cpp
        activityservice.cpp
        activityjob.cpp)

set_source_files_properties(org.kde.ActivityManager.ActivityRanking.xml PROPERTIES INCLUDE "ActivityData.h")
qt5_add_dbus_interface(
   activity_engine_SRCS org.kde.ActivityManager.ActivityRanking.xml
   ActivityRankingInterface
   )

add_library(plasma_engine_activities MODULE ${activity_engine_SRCS})
target_link_libraries(plasma_engine_activities
                      KF5::CoreAddons
                      KF5::Plasma
                      KF5::Activities
                      KF5::I18n
                      KF5::Service
                      Qt::DBus
                      Qt::Widgets
                      )
kcoreaddons_desktop_to_json(plasma_engine_activities plasma-dataengine-activities.desktop)

install(TARGETS plasma_engine_activities
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

install(FILES plasma-dataengine-activities.desktop
        DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES activities.operations
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
