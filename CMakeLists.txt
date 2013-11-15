project(plasma-shell)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

add_definitions(-DHAVE_X11=${HAVE_X11})

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(KCoreAddons REQUIRED)
find_package(Solid REQUIRED)
find_package(KActivities REQUIRED)

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
 #Qt5::Widgets
 Qt5::Quick
 Qt5::Qml
 KF5::KIOCore
 KF5::KWindowSystem
 KF5::KCoreAddons
 Plasma
 PlasmaQuick
 Qt5::Script
 KF5::KDE4Support
 KF5::Solid
 KF5::KDeclarative
 KF5::KI18n
 KF5::XmlGui
 KF5::KIconThemes
 KF5::KActivities
)
target_include_directories(plasma-shell PRIVATE "${CMAKE_BINARY_DIR}")

if(X11_FOUND)
  target_link_libraries(plasma-shell ${X11_LIBRARIES} ${XCB_XCB_LIBRARY} )
  target_link_libraries(plasma-shell Qt5::X11Extras)
endif()

install(TARGETS plasma-shell ${INSTALL_TARGETS_DEFAULT_ARGS})

