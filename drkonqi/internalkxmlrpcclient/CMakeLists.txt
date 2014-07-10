cmake_minimum_required(VERSION 2.8.12)

project(KXmlRpcClientPrivate)

# ECM setup
find_package(ECM 1.0.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

set(KF5_MIN_VERSION "5.0.0")
set(KDEPIMLIBS_VERSION "4.70.0")

add_definitions(-DTRANSLATION_DOMAIN=\"libkxmlrpcclientprivate5\")

# FIXME: KXmlRpcClient ? KXMLRPCClient ?
ecm_setup_version(${KDEPIMLIBS_VERSION} VARIABLE_PREFIX KXMLRPCCLIENTPRIVATE
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kxmlrpcclientprivate_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5XmlRpcClientPrivateConfigVersion.cmake"
                        SOVERSION 5
)

########### Find packages ###########
find_package(KF5KIO ${KF5_MIN_VERSION} CONFIG REQUIRED)

########### Targets ###########
add_subdirectory(src)
add_subdirectory(autotests)

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5XmlRpcClientPrivate")

ecm_configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5XmlRpcClientPrivateConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5XmlRpcClientPrivateConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

# install(FILES
#   "${CMAKE_CURRENT_BINARY_DIR}/KF5XmlRpcClientPrivateConfig.cmake"
#   "${CMAKE_CURRENT_BINARY_DIR}/KF5XmlRpcClientPrivateConfigVersion.cmake"
#   DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
#   COMPONENT Devel
# )

# install(EXPORT KF5XmlRpcClientPrivateTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5XmlRpcClientPrivateTargets.cmake)

# install(FILES
#    ${CMAKE_CURRENT_BINARY_DIR}/kxmlrpcclientprivate_version.h
#   DESTINATION ${KF5_INCLUDE_INSTALL_DIR}
#   COMPONENT Devel
# )

if ("${CMAKE_BINARY_DIR}" STREQUAL "${KXmlRpcClientPrivate_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
