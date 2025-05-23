add_subdirectory(kded)

set(colorcorrect_LIB_SRCS
    compositorcoloradaptor.cpp
    suncalc.cpp
    )

add_library(colorcorrect ${colorcorrect_LIB_SRCS})
add_library(PW::LibColorCorrect ALIAS colorcorrect)

generate_export_header(colorcorrect)

target_include_directories(colorcorrect PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/colorcorrect>")

target_link_libraries(colorcorrect
                    PUBLIC
                      KF6::ConfigCore

                      Qt::Core
                      Qt::Quick
                    PRIVATE
                      KF6::Holidays
                      KF6::I18n

                      Qt::DBus)

set_target_properties(colorcorrect PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR}
                      EXPORT_NAME LibColorCorrect)

ecm_add_qml_module(colorcorrect URI org.kde.colorcorrect GENERATE_PLUGIN_SOURCE)
ecm_finalize_qml_module(colorcorrect)

install(TARGETS colorcorrect EXPORT libcolorcorrectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    colorcorrectconstants.h
    compositorcoloradaptor.h
    ${CMAKE_CURRENT_BINARY_DIR}/colorcorrect_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/colorcorrect COMPONENT Devel
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibColorCorrectConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/LibColorCorrect)

configure_package_config_file(LibColorCorrectConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibColorCorrectConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibColorCorrectConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/LibColorCorrectConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libcolorcorrectLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibColorCorrectLibraryTargets.cmake )
