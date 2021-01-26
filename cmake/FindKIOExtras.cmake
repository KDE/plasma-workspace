find_path(KIOExtras_PATH thumbnail.protocol PATHS ${KDE_INSTALL_FULL_KSERVICES5DIR})

if (KIOExtras_PATH)
    set(KIOExtras_FOUND TRUE)
endif()
