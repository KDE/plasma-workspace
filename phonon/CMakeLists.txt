# If you want to build kdebase without any multimedia support
# define the cmake variable KDEBASE_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDEBASE_DISABLE_MULTIMEDIA=ON
#
project(PHONON-KDE-RUNTIME)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

find_package(Phonon4Qt5 4.6.60 NO_MODULE)
include_directories(BEFORE ${PHONON_INCLUDES})

option(KDEBASE_DISABLE_MULTIMEDIA "Disable multimedia support (default: off)" OFF)
if(KDE4_DISABLE_MULTIMEDIA)
   message(STATUS "*********** The switch KDE4_DISABLE_MULTIMEDIA is not supported anymore, use KDEBASE_DISABLE_MULTIMEDIA instead ************")
endif()


if(KDEBASE_DISABLE_MULTIMEDIA)
   message(STATUS "NOTICE: Multimedia support DISABLED (KDEBASE_DISABLE_MULTIMEDIA == ON)")
else()
   find_package(Alsa)
   option(WITH_ALSA "Use ALSA, if found" ON)
   if(NOT WITH_ALSA)
      set(ALSA_FOUND FALSE)
      set(ASOUND_LIBRARY NOTFOUND)
      set(HAVE_LIBASOUND2 FALSE)
      set(ALSA_INCLUDES NOTFOUND)
   endif()

   alsa_configure_file(${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

   set(HAVE_CURRENT_ALSA FALSE)
   if(ALSA_FOUND)
      include_directories(${ALSA_INCLUDES})
      alsa_version_string(ALSA_VERSION_STR)
      if(ALSA_VERSION_STR AND NOT WIN32)
         if(ALSA_VERSION_STR MATCHES "^1\\.(0\\.(1[4-9]|[2-9][0-9]+)|[1-9][0-9]*\\.)")
            add_definitions(-DHAS_LIBASOUND_DEVICE_NAME_HINT)
            set(HAVE_CURRENT_ALSA TRUE)
         endif()
      endif()
   endif()
   if(NOT WIN32)
      add_feature_info("Dmix and virtual device listing" HAVE_CURRENT_ALSA
                       "The ALSA library (http://www.alsa-project.org) is needed for building
                        some additional Phonon features"
                      )
   endif()

   add_subdirectory(kded-module)
   add_subdirectory(platform_kde)
endif()
