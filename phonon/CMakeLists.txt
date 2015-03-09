# If you want to build kdebase without any multimedia support
# define the cmake variable KDEBASE_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDEBASE_DISABLE_MULTIMEDIA=ON
#
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
    add_subdirectory(platform_kde)
endif()
