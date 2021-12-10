set(activity_engine_SRCS
        ActivityData.cpp
        activityengine.cpp
        activityservice.cpp
        activityjob.cpp)

set_source_files_properties(org.kde.ActivityManager.ActivityRanking.xml PROPERTIES INCLUDE "ActivityData.h")
qt_add_dbus_interface(
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

install(TARGETS plasma_engine_activities
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)

install(FILES activities.operations
        DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
