add_executable(kfontview_bin Viewer.cpp)
set_target_properties(kfontview_bin PROPERTIES OUTPUT_NAME kfontview)
target_link_libraries(kfontview_bin KF6::Parts KF6::XmlGui KF6::DBusAddons kfontinstui kfontinst)

ecm_qt_declare_logging_category(kfontview_bin
    HEADER kfontview_debug.h
    IDENTIFIER KFONTVIEW_DEBUG
    CATEGORY_NAME org.kde.plasma.kfontview
    DESCRIPTION "kcm kfontview"
    EXPORT PLASMAWORKSPACE
)

install(TARGETS kfontview_bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES kfontviewui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kfontview )
install(FILES org.kde.kfontview.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
