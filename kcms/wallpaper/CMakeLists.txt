add_definitions(-DTRANSLATION_DOMAIN=\"kcm_wallpaper\")

ki18n_wrap_ui(kcm_wallpaper)

kcmutils_add_qml_kcm(kcm_wallpaper SOURCES ${kcm_wallpaper_PART_SRCS})
target_sources(kcm_wallpaper PRIVATE
    wallpapermodule.cpp
    wallpapermodule.h
    ${CMAKE_CURRENT_BINARY_DIR}/../../config-workspace.h
    )

ecm_qt_declare_logging_category(kcm_wallpaper
    HEADER kcm_wallpaper_debug.h
    IDENTIFIER KCM_WALLPAPER_DEBUG
    CATEGORY_NAME kcm_wallpaper
    DESCRIPTION "kcm wallpaper"
    EXPORT PLASMAWORKSPACE
)

target_link_libraries(kcm_wallpaper PRIVATE
    Qt6::Quick
    Qt6::DBus
    KF6::I18n
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KCMUtilsQuick
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ConfigQml
    KF6::Package
    Plasma::Plasma
    Plasma::PlasmaQuick
    Plasma::Activities
    PW::KWorkspace
)

