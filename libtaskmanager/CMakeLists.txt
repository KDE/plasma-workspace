add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1204)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

########### next target ###############

set(taskmanager_LIB_SRCS
   abstractgroupableitem.cpp
   abstractgroupingstrategy.cpp
   abstractsortingstrategy.cpp
   groupmanager.cpp
   launcheritem.cpp
   startup.cpp
   strategies/activitysortingstrategy.cpp
   strategies/alphasortingstrategy.cpp
   strategies/desktopsortingstrategy.cpp
   strategies/programgroupingstrategy.cpp
   strategies/manualgroupingstrategy.cpp
   strategies/manualsortingstrategy.cpp
   task.cpp
   taskactions.cpp
   taskgroup.cpp
   taskitem.cpp
   taskmanager.cpp
   tasksmodel.cpp
   launcherconfig.cpp
   launcherproperties.cpp
   )

qt5_wrap_ui(taskmanager_LIB_SRCS launcherconfig.ui launcherproperties.ui)

if(WIN32)
    set(taskmanager_LIB_SRCS ${taskmanager_LIB_SRCS} task_win.cpp)
else()
    set(taskmanager_LIB_SRCS ${taskmanager_LIB_SRCS} task_x11.cpp)
endif()

add_library(taskmanager ${taskmanager_LIB_SRCS})
add_library(PW::LibTaskManager ALIAS taskmanager)
generate_export_header(taskmanager)
target_include_directories(taskmanager PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/taskmanager>")
target_link_libraries(taskmanager
    PUBLIC
        Qt5::Core
        Qt5::Gui
        KF5::WindowSystem # KStartupInfo
    PRIVATE
        KF5::ProcessCore
        KF5::ConfigCore
        KF5::ConfigWidgets
        KF5::IconThemes
        KF5::I18n
        KF5::KIOCore
        KF5::KIOWidgets # KRun
        KF5::Service
)
if (X11_FOUND)
    target_link_libraries(taskmanager PRIVATE ${Qt5X11Extras_LIBRARIES} ${X11_LIBRARIES})
    if (X11_Xfixes_FOUND)
    target_link_libraries(taskmanager PRIVATE ${X11_Xfixes_LIB})
    endif ()
    if (X11_Xrender_FOUND)
    target_link_libraries(taskmanager PRIVATE ${X11_Xrender_LIB})
    endif ()
    if (X11_Xcomposite_FOUND)
    target_link_libraries(taskmanager PRIVATE ${X11_Xcomposite_LIB})
    endif ()
endif()

set_target_properties(taskmanager PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} EXPORT_NAME LibTaskManager)
install(TARGETS taskmanager EXPORT libtaskmanagerLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
   abstractgroupableitem.h
   abstractgroupingstrategy.h
   abstractsortingstrategy.h
   groupmanager.h
   launcheritem.h
   startup.h
   task.h
   taskactions.h
   taskgroup.h
   taskitem.h
   taskmanager.h
   tasksmodel.h
   ${CMAKE_CURRENT_BINARY_DIR}/taskmanager_export.h
   DESTINATION ${INCLUDE_INSTALL_DIR}/taskmanager COMPONENT Devel
   )

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfigVersion.cmake VERSION "${PLASMAWORKSPACE_VERSION_STRING}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/LibTaskManager)
ecm_configure_package_config_file(LibTaskManagerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libtaskmanagerLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibTaskManagerLibraryTargets.cmake )


#set(tasksmodel_SRCS 
#   modeltest/dynamictreemodel.cpp
#   modeltest/modeltest.cpp
#   modeltest/main.cpp
#   )

#add_executable(tasksmodeltest ${tasksmodel_SRCS})
#target_link_libraries(tasksmodeltest ${KDE4_KDEUI_LIBS} KF5::KIOCore KF5::KIOWidgets ${X11_LIBRARIES} taskmanager kephal)


