find_path(KIOExtras_PATH thumbnail.so PATHS ${KDE_INSTALL_FULL_PLUGINDIR}/kf5/kio/)

if (KIOExtras_PATH)
    set(KIOExtras_FOUND TRUE)
endif()
