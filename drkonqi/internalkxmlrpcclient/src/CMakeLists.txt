set(kxmlrpcclient_SRCS
  client.cpp
  query.cpp
)

add_library(KF5XmlRpcClientPrivate SHARED ${kxmlrpcclient_SRCS})

generate_export_header(KF5XmlRpcClientPrivate BASE_NAME kxmlrpcclientprivate)

add_library(XmlRpcClientPrivate ALIAS KF5XmlRpcClientPrivate)

target_include_directories(KF5XmlRpcClientPrivate INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KXmlRpcClientPrivate;${KF5_INCLUDE_INSTALL_DIR}/KXmlRpcClientPrivate/kxmlrpcclientprivate>")

target_link_libraries(KF5XmlRpcClientPrivate 
PUBLIC 
   KF5::KIOCore 
PRIVATE
   KF5::I18n
   Qt5::Xml
)

set_target_properties(KF5XmlRpcClientPrivate PROPERTIES
    VERSION ${KXMLRPCCLIENTPRIVATE_VERSION_STRING}
    SOVERSION ${KXMLRPCCLIENTPRIVATE_SOVERSION}
    EXPORT_NAME XmlRpcClientPrivate
)

ecm_generate_headers(KXmlRpcClientPrivate_CamelCase_HEADERS
  HEADER_NAMES
  Client
  PREFIX KXmlRpcClient
  REQUIRED_HEADERS KXmlRpcClientPrivate_HEADERS
)

install(TARGETS KF5XmlRpcClientPrivate EXPORT KF5XmlRpcClientPrivateTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})


# ecm_generate_pri_file(BASE_NAME KXmlRpcClientPrivate
#     LIB_NAME KF5XmlRpcClientPrivate
#     DEPS "KIOCore" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KF5_INCLUDE_INSTALL_DIR}/KXmlRpcClientPrivate
# )
# 
# install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
# 
# 
# install(FILES ${KXmlRpcClientPrivate_CamelCase_HEADERS}
#   DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KXmlRpcClientPrivate/KXmlRpcClientPrivate/)
# 
# install(FILES
#   ${CMAKE_CURRENT_BINARY_DIR}/kxmlrpcclientprivate_export.h
#   ${KXmlRpcClient_HEADERS}
#   DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KXmlRpcClientPrivate/kxmlrpcclientprivate/
#   COMPONENT Devel
# )
