add_subdirectory(declarative)
add_subdirectory(kded)
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

set(colorcorrect_LIB_SRCS
    compositorcoloradaptor.cpp
    geolocator.cpp
    suncalc.cpp
    )

add_library(colorcorrect ${colorcorrect_LIB_SRCS})

generate_export_header(colorcorrect)

target_include_directories(colorcorrect PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/colorcorrect>")

target_link_libraries(colorcorrect
                    PUBLIC
                      KF5::Plasma

                      Qt::Core
                      Qt::Quick
                    PRIVATE
                      KF5::I18n

                      Qt::DBus)

set_target_properties(colorcorrect PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR}
                      EXPORT_NAME LibColorCorrect)

install(TARGETS colorcorrect EXPORT libcolorcorrectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    colorcorrectconstants.h
    compositorcoloradaptor.h
    geolocator.h
    ${CMAKE_CURRENT_BINARY_DIR}/colorcorrect_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/colorcorrect COMPONENT Devel
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibColorCorrectConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_LIBDIR}/cmake/LibColorCorrect)

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
