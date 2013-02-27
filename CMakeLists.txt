project(testplasma2)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})


find_path(KDE_MODULES_DIR NAMES KDE4Macros.cmake PATH_SUFFIXES share/cmake/modules)


add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})


find_package(Qt5Transitional REQUIRED Core)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)


# The Qt5Widgets_INCLUDES also includes the include directories for
# dependencies QtCore and QtGui
include_directories(${Qt5Widgets_DEFINITIONS} ${Qt5Quick_DEFINITIONS} ${QT_INCLUDES} ${KDE4_INCLUDES})

# We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
add_definitions(${Qt5Widgets_DEFINITIONS} ${Qt5Quick_DEFINITIONS} ${Qt5Qml_DEFINITIONS})

# Executables fail to build with Qt 5 in the default configuration
# without -fPIE. We add that here.
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

add_executable(testplasma2
    main.cpp
    desktopcorona.cpp
    panelview.cpp
    view.cpp
)

# The Qt5Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries(testplasma2
 #${Qt5Widgets_LIBRARIES}
 ${Qt5Quick_LIBRARIES}
 ${Qt5Qml_LIBRARIES}
 ${KDE4_KDECORE_LIBS}
 ${KWindowSystem_LIBRARIES}
 plasma
)

install(TARGETS testplasma2 ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(applets)
add_subdirectory(containments)
add_subdirectory(qmlpackages)
