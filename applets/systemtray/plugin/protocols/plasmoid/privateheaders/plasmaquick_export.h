
#ifndef PLASMAQUICK_EXPORT_H
#define PLASMAQUICK_EXPORT_H

#ifdef PLASMAQUICK_STATIC_DEFINE
#  define PLASMAQUICK_EXPORT
#  define PLASMAQUICK_NO_EXPORT
#else
#  ifndef PLASMAQUICK_EXPORT
#    ifdef KF5PlasmaQuick_EXPORTS
        /* We are building this library */
#      define PLASMAQUICK_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define PLASMAQUICK_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef PLASMAQUICK_NO_EXPORT
#    define PLASMAQUICK_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef PLASMAQUICK_DEPRECATED
#  define PLASMAQUICK_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef PLASMAQUICK_DEPRECATED_EXPORT
#  define PLASMAQUICK_DEPRECATED_EXPORT PLASMAQUICK_EXPORT PLASMAQUICK_DEPRECATED
#endif

#ifndef PLASMAQUICK_DEPRECATED_NO_EXPORT
#  define PLASMAQUICK_DEPRECATED_NO_EXPORT PLASMAQUICK_NO_EXPORT PLASMAQUICK_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define PLASMAQUICK_NO_DEPRECATED
#endif

#endif
