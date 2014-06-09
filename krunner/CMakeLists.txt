
find_package(X11 MODULE)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                       URL "http://www.x.org"
                       TYPE OPTIONAL
                      )
find_package(XCB MODULE COMPONENTS XCB COMPOSITE DAMAGE SHAPE)
set_package_properties(XCB PROPERTIES DESCRIPTION "X protocol C-language Binding"
                       URL "http://xcb.freedesktop.org"
                       TYPE OPTIONAL
                      )

if(X11_FOUND AND XCB_XCB_FOUND)
  find_package(Qt5 REQUIRED NO_MODULE COMPONENTS X11Extras)
  #X11_Xrender discovery is done by FindX11
  #add_feature_info("X Rendering Extension (libXrender)" X11_Xrender_FOUND "Support for compositing, rendering operations, and alpha-blending. STRONGLY RECOMMENDED")
endif()

if(X11_FOUND AND XCB_XCB_FOUND AND XCB_COMPOSITE_FOUND AND XCB_DAMAGE_FOUND)
    add_definitions(-DHAVE_XCB_COMPOSITE=1)
    include_directories(
        ${XCB_XCB_INCLUDE_DIR}
        ${XCB_DAMAGE_INCLUDE_DIR}
        ${XCB_COMPOSITE_INCLUDE_DIR}
    )
else()
    add_definitions(-DHAVE_XCB_COMPOSITE=0)
endif()
if(X11_FOUND AND XCB_XCB_FOUND AND XCB_SHAPE_FOUND)
    add_definitions(-DHAVE_XCB_SHAPE=1)
    include_directories(${XCB_SHAPE_INCLUDE_DIR})
else()
    add_definitions(-DHAVE_XCB_SHAPE=0)
endif()

set(krunner_SRCS
    #    fullview.cpp
    main.cpp
    view.cpp
)

find_package(KF5Declarative ${KF5_VERSION} REQUIRED)
find_package(KF5Plasma ${KF5_VERSION} REQUIRED)
find_package(KF5PlasmaQuick REQUIRED NO_MODULE)
find_package(KF5Config REQUIRED NO_MODULE)

set(krunner_dbusAppXML dbus/org.kde.krunner.App.xml)
qt5_add_dbus_adaptor(krunner_SRCS ${krunner_dbusAppXML} view.h View)
configure_file(dbus/org.kde.krunner.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.krunner.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})


add_executable(krunner ${krunner_SRCS})

target_link_libraries(krunner
    Qt5::Widgets
    Qt5::Quick
    KF5::Declarative
    KF5::I18n
    KF5::Plasma
    KF5::PlasmaQuick
    KF5::GlobalAccel
    KF5::DBusAddons
    KF5::WindowSystem
    KF5::ConfigWidgets
)

if(X11_FOUND)
  target_link_libraries(krunner ${X11_LIBRARIES} ${XCB_XCB_LIBRARY} )
  target_link_libraries(krunner Qt5::X11Extras)

  if(XCB_COMPOSITE_FOUND AND XCB_DAMAGE_FOUND)
    target_link_libraries(krunner
        ${XCB_DAMAGE_LIBRARY}
        ${XCB_COMPOSITE_LIBRARY}
        )
  endif()
  if(XCB_SHAPE_FOUND)
    target_link_libraries(krunner ${XCB_SHAPE_LIBRARY})
  endif()
endif()

install(TARGETS krunner ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${krunner_dbusAppXML} DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KRunnerAppDBusInterface")
ecm_configure_package_config_file(KRunnerAppDBusInterfaceConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KRunnerAppDBusInterfaceConfig.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})
