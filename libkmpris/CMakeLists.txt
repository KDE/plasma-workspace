# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"libkmpris\")

set(libkmpris_SRCS
    mpris2sourcemodel.cpp
    mpris2filterproxymodel.cpp
    mpris2model.cpp
    multiplexer.cpp
    multiplexermodel.cpp
    playercontainer.cpp
)

ecm_qt_declare_logging_category(libkmpris_SRCS HEADER libkmpris_debug.h
    IDENTIFIER MPRIS2
    CATEGORY_NAME kde.dataengine.mpris
    DEFAULT_SEVERITY Info)

set_source_files_properties(
   dbus/org.freedesktop.DBus.Properties.xml
   dbus/org.mpris.MediaPlayer2.Player.xml
   dbus/org.mpris.MediaPlayer2.xml
   PROPERTIES
   NO_NAMESPACE ON)
qt_add_dbus_interface(libkmpris_SRCS dbus/org.freedesktop.DBus.Properties.xml dbusproperties)
qt_add_dbus_interface(libkmpris_SRCS dbus/org.mpris.MediaPlayer2.Player.xml mprisplayer)
qt_add_dbus_interface(libkmpris_SRCS dbus/org.mpris.MediaPlayer2.xml mprisroot)

add_library(kmpris SHARED ${libkmpris_SRCS})
generate_export_header(kmpris)
target_include_directories(kmpris PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(kmpris
    PUBLIC
        Qt::Core
    PRIVATE
        Qt::DBus
        KF6::ConfigCore
        KF6::I18n
)
set_target_properties(kmpris PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS kmpris ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(declarative)
add_subdirectory(kded)

if(BUILD_TESTING)
add_subdirectory(autotests)
endif()
