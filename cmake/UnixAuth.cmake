find_package(PAM)

include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CMakePushCheckState)

set(UNIXAUTH_LIBRARIES)
set(UNIXAUTH_INCLUDE_DIRS)

set(SHADOW_LIBRARIES)
check_function_exists(getspnam found_getspnam)
if (found_getspnam)
    set(HAVE_GETSPNAM 1)
else (found_getspnam)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_LIBRARIES -lshadow)
    check_function_exists(getspnam found_getspnam_shadow)
    if (found_getspnam_shadow)
        set(HAVE_GETSPNAM 1)
        set(SHADOW_LIBRARIES shadow)
        check_function_exists(pw_encrypt HAVE_PW_ENCRYPT) # ancient Linux shadow
    else (found_getspnam_shadow)
        set(CMAKE_REQUIRED_LIBRARIES -lgen) # UnixWare
        check_function_exists(getspnam found_getspnam_gen)
        if (found_getspnam_gen)
            set(HAVE_GETSPNAM 1)
            set(SHADOW_LIBRARIES gen)
        endif (found_getspnam_gen)
    endif (found_getspnam_shadow)
    cmake_pop_check_state()
endif (found_getspnam)

set(CRYPT_LIBRARIES)
check_library_exists(crypt crypt "" HAVE_CRYPT)
if (HAVE_CRYPT)
    set(CRYPT_LIBRARIES crypt)
    check_include_files(crypt.h HAVE_CRYPT_H)
endif (HAVE_CRYPT)

if (PAM_FOUND)

    set(HAVE_PAM 1)
    set(UNIXAUTH_LIBRARIES ${PAM_LIBRARIES})
    set(UNIXAUTH_INCLUDE_DIRS ${PAM_INCLUDE_DIR})

else (PAM_FOUND)

    if (HAVE_GETSPNAM)
        set(UNIXAUTH_LIBRARIES ${SHADOW_LIBRARIES})
    endif (HAVE_GETSPNAM)
    if (NOT HAVE_PW_ENCRYPT)
        set(UNIXAUTH_LIBRARIES ${UNIXAUTH_LIBRARIES} ${CRYPT_LIBRARIES})
    endif (NOT HAVE_PW_ENCRYPT)

endif (PAM_FOUND)
