add_subdirectory(declarative)
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

set(taskmanager_LIB_SRCS
    abstracttasksmodel.cpp
    abstracttasksproxymodeliface.cpp
    abstractwindowtasksmodel.cpp
    activityinfo.cpp
    concatenatetasksproxymodel.cpp
    flattentaskgroupsproxymodel.cpp
    launchertasksmodel.cpp
    startuptasksmodel.cpp
    regionfiltermode.h
    taskfilterproxymodel.cpp
    taskgroupingproxymodel.cpp
    tasksmodel.cpp
    tasktools.cpp
    virtualdesktopinfo.cpp
    waylandstartuptasksmodel.cpp
    waylandtasksmodel.cpp
    windowtasksmodel.cpp
)

if (HAVE_X11)
    set(taskmanager_LIB_SRCS
        ${taskmanager_LIB_SRCS}
        xstartuptasksmodel.cpp
        xwindowsystemeventbatcher.cpp
        xwindowtasksmodel.cpp
    )
endif()

add_library(taskmanager ${taskmanager_LIB_SRCS})
add_library(PW::LibTaskManager ALIAS taskmanager)

generate_export_header(taskmanager)

target_include_directories(taskmanager PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/taskmanager>")

target_link_libraries(taskmanager
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Quick
        KF6::ItemModels
    PRIVATE
        Qt::DBus
        KF6::Activities
        KF6::ConfigCore
        KF6::I18n
        KF6::KIOCore
        KF6::KIOGui
        KF6::JobWidgets
        KF6::WaylandClient
        KF6::WindowSystem
)

if (HAVE_X11)
    target_link_libraries(taskmanager PRIVATE KF6::IconThemes)
    target_link_libraries(taskmanager PRIVATE Qt::GuiPrivate)
endif()

set_target_properties(taskmanager PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION 6
                      EXPORT_NAME LibTaskManager)


ecm_qt_declare_logging_category(taskmanager
    HEADER libtaskmanager_debug.h
    IDENTIFIER TASKMANAGER_DEBUG
    CATEGORY_NAME org.kde.plasma.libtaskmanager
    DESCRIPTION "libtaskmanager debug"
    EXPORT PLASMAWORKSPACE
)

install(TARGETS taskmanager EXPORT libtaskmanagerLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    abstracttasksmodel.h
    abstracttasksmodeliface.h
    abstracttasksproxymodeliface.h
    abstractwindowtasksmodel.h
    activityinfo.h
    concatenatetasksproxymodel.h
    flattentaskgroupsproxymodel.h
    launchertasksmodel.h
    regionfiltermode.h
    startuptasksmodel.h
    taskfilterproxymodel.h
    taskgroupingproxymodel.h
    tasksmodel.h
    tasktools.h
    virtualdesktopinfo.h
    waylandstartuptasksmodel.h
    waylandtasksmodel.h
    windowtasksmodel.h
    ${CMAKE_CURRENT_BINARY_DIR}/taskmanager_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/taskmanager COMPONENT Devel
)

if (HAVE_X11)
    install(FILES
        xstartuptasksmodel.h
        xwindowtasksmodel.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/taskmanager COMPONENT Devel
    )
endif()

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/LibTaskManager)

configure_package_config_file(LibTaskManagerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libtaskmanagerLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibTaskManagerLibraryTargets.cmake )

install(FILES taskmanagerrulesrc
        DESTINATION ${KDE_INSTALL_CONFDIR})
