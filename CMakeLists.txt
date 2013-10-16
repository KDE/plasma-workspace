project(plasma-shell)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})


find_path(KDE_MODULES_DIR NAMES KDE4Macros.cmake PATH_SUFFIXES share/cmake/modules)

add_definitions(-DHAVE_X11=${HAVE_X11})

find_package(Qt5Transitional REQUIRED Core)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(KCoreAddons REQUIRED)
find_package(Solid REQUIRED)

set(scripting_SRC
    scripting/appinterface.cpp
    scripting/applet.cpp
    scripting/containment.cpp
    scripting/configgroup.cpp
    scripting/desktopscriptengine.cpp
    scripting/i18n.cpp
    scripting/layouttemplatepackagestructure.cpp
    scripting/panel.cpp
    scripting/rect.cpp
    scripting/scriptengine.cpp
    scripting/widget.cpp
)

set(widgetexplorer_SRC
    widgetexplorer/kcategorizeditemsviewmodels.cpp
    widgetexplorer/plasmaappletitemmodel.cpp
    widgetexplorer/widgetexplorer.cpp
    widgetexplorer/widgetexplorerview.cpp
)

add_executable(plasma-shell
    activity.cpp
    main.cpp
    containmentconfigview.cpp
    currentcontainmentactionsmodel.cpp
    desktopview.cpp
    kidenticongenerator.cpp
    panelview.cpp
    panelconfigview.cpp
    panelshadows.cpp
    packages.cpp
    shellcorona.cpp
    shellpluginloader.cpp
    shellmanager.cpp
    ${scripting_SRC}
    ${widgetexplorer_SRC}
)

# The Qt5Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries(plasma-shell
 #${Qt5Widgets_LIBRARIES}
 ${Qt5Quick_LIBRARIES}
 ${Qt5Qml_LIBRARIES}
 ${KCore_LIBRARIES}
 KF5::KIOCore
 ${KWindowSystem_LIBRARIES}
 KF5::KCoreAddons
 Plasma
 PlasmaQuick
 ${Qt5Script_LIBRARIES}
 ${KDE4Support_LIBRARIES}
 KF5::Solid
 KF5::KDeclarative
 KF5::KI18n
 KF5::XmlGui
 KF5::KIconThemes
 /opt/kde5/lib64/libkactivities.so
)
target_include_directories(plasma-shell PRIVATE "${CMAKE_BINARY_DIR}")

if(X11_FOUND)
  target_link_libraries(plasma-shell ${X11_LIBRARIES} ${XCB_XCB_LIBRARY} )
  target_link_libraries(plasma-shell Qt5::X11Extras)
endif(X11_FOUND)

install(TARGETS plasma-shell ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(qmlpackages)
