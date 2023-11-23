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
                      KF6::CoreAddons
                      KF6::Plasma
                      KF6::Plasma5Support
                      Plasma::Activities
                      KF6::I18n
                      KF6::Service
                      Qt::DBus
                      Qt::Widgets
                      )

install(TARGETS plasma_engine_activities
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)

install(FILES activities.operations
        DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
