find_program(KIOFuse_PATH kio-fuse PATHS ${KDE_INSTALL_FULL_LIBEXECDIR})

if (KIOFuse_PATH)
    set(KIOFuse_FOUND TRUE)
endif()
