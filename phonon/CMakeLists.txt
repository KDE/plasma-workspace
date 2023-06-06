add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)

find_package(Phonon4Qt6 4.6.60 NO_MODULE)
include_directories(BEFORE ${PHONON_INCLUDES})

if(Phonon4Qt6_FOUND)
    add_subdirectory(platform_kde)
endif()
