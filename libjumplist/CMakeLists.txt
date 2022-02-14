add_subdirectory(declarative)
# if(BUILD_TESTING)
#    add_subdirectory(autotests)
# endif()

set(jumplist_LIB_SRCS
    jumplist.cpp
)

ecm_qt_declare_logging_category(jumplist_LIB_SRCS
    HEADER log_settings.h
    IDENTIFIER JUMPLIST_DEBUG
    CATEGORY_NAME org.kde.plasma.jumplist)

add_library(jumplist ${jumplist_LIB_SRCS})
add_library(PW::LibJumpList ALIAS jumplist)

generate_export_header(jumplist)

target_include_directories(jumplist PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/jumplist>")

target_link_libraries(jumplist
    PUBLIC
        Qt::Core
        Qt::Gui
    PRIVATE
        Qt::Widgets
        KF5::Activities
        KF5::ActivitiesStats
        KF5::ConfigCore # KDesktopFile
        KF5::I18n
        KF5::KIOCore
        KF5::KIOFileWidgets # KFilePlacesModel
        KF5::KIOGui
        KF5::Notifications # KNotificationJobUiDelegate
)

set_target_properties(jumplist PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION 1
                      EXPORT_NAME LibJumpList)

install(TARGETS jumplist EXPORT libjumplistLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    jumplist.h
    ${CMAKE_CURRENT_BINARY_DIR}/jumplist_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/jumplist COMPONENT Devel
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibJumpListConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_LIBDIR}/cmake/LibJumpList)

configure_package_config_file(LibJumpListConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibJumpListConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibJumpListConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/LibJumpListConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libjumplistLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibJumpListLibraryTargets.cmake )
