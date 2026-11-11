configure_file(config-paths.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h)

add_library(kfontinst SHARED
    Misc.cpp
    Fc.cpp
    Family.cpp
    Style.cpp
    File.cpp
    WritingSystems.cpp
)

target_link_libraries(kfontinst
    PUBLIC
        KF6::I18n
        KF6::KIOCore
        Fontconfig::Fontconfig
        Freetype::Freetype
    PRIVATE
        Qt::Xml
        Qt::Gui
)

set_target_properties(kfontinst PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

generate_export_header(kfontinst)

add_library(kfontinstui SHARED FcEngine.cpp ActionLabel.cpp)
target_link_libraries(kfontinstui PUBLIC
    Qt::GuiPrivate
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::IconThemes
    X11::X11
    X11::Xft
    kfontinst
)

set_target_properties(kfontinstui PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS kfontinst kfontinstui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
