set(kfontinst_LIB_SRCS Misc.cpp Fc.cpp Family.cpp Style.cpp File.cpp WritingSystems.cpp)
set(kfontinstui_LIB_SRCS FcEngine.cpp )

configure_file(config-paths.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h)

add_library(kfontinst SHARED ${kfontinst_LIB_SRCS})
target_link_libraries(kfontinst
    PUBLIC
    KF5::I18n
    KF5::KIOCore
    Fontconfig::Fontconfig
    PRIVATE
    Qt::Xml
    Qt::Gui
    XCB::IMAGE
)

set_target_properties(kfontinst PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

add_library(kfontinstui SHARED ${kfontinstui_LIB_SRCS})
target_link_libraries(kfontinstui Qt::X11Extras KF5::KIOCore KF5::KIOWidgets X11::X11 X11::Xft XCB::IMAGE kfontinst )
set_target_properties(kfontinstui PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )
install(TARGETS kfontinst kfontinstui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
