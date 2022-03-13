add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_mpris2\")
add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)

set(mpris2_engine_SRCS
    mpris2engine.cpp
    multiplexer.cpp
    multiplexedservice.cpp
    playercontrol.cpp
    playeractionjob.cpp
    playercontainer.cpp
    mpris2engine.h
    multiplexer.h
    multiplexedservice.h
    playercontrol.h
    playeractionjob.h
    playercontainer.h
)

ecm_qt_declare_logging_category(mpris2_engine_SRCS HEADER debug.h
                                               IDENTIFIER MPRIS2
                                               CATEGORY_NAME kde.dataengine.mpris
                                               DEFAULT_SEVERITY Info)

set_source_files_properties(
   org.freedesktop.DBus.Properties.xml
   org.mpris.MediaPlayer2.Player.xml
   org.mpris.MediaPlayer2.xml
   PROPERTIES
   NO_NAMESPACE ON)
qt_add_dbus_interface(mpris2_engine_SRCS org.freedesktop.DBus.Properties.xml dbusproperties)
qt_add_dbus_interface(mpris2_engine_SRCS org.mpris.MediaPlayer2.Player.xml mprisplayer)
qt_add_dbus_interface(mpris2_engine_SRCS org.mpris.MediaPlayer2.xml mprisroot)

kcoreaddons_add_plugin(plasma_engine_mpris2 SOURCES ${mpris2_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_link_libraries(plasma_engine_mpris2
   Qt::DBus
   KF5::ConfigCore
   KF5::GlobalAccel
   KF5::I18n
   KF5::Service
   KF5::Plasma
   KF5::XmlGui
)

install(FILES mpris2.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
