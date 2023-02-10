set(scheme_editor_SRCS
    kcolorschemeeditor.cpp
    scmeditordialog.cpp
    scmeditoroptions.cpp
    scmeditorcolors.cpp
    scmeditoreffects.cpp
    previewwidget.cpp
    setpreviewwidget.cpp
    scmeditordialog.h
    scmeditoroptions.h
    scmeditorcolors.h
    scmeditoreffects.h
    previewwidget.h
    setpreviewwidget.h
)

ki18n_wrap_ui(scheme_editor_SRCS
    scmeditordialog.ui
    scmeditoroptions.ui
    scmeditorcolors.ui
    scmeditoreffects.ui
    preview.ui
    setpreview.ui
)

add_executable(kcolorschemeeditor ${scheme_editor_SRCS})

target_link_libraries(kcolorschemeeditor
    KF6::ConfigWidgets
    KF6::GuiAddons
    KF6::I18n
    KF6::CoreAddons
    KF6::NewStuff
    KF6::WindowSystem
)

install(TARGETS kcolorschemeeditor DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kcolorschemeeditor.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
