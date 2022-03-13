add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)

find_package(Phonon4Qt${QT_MAJOR_VERSION} 4.6.60 NO_MODULE)
include_directories(BEFORE ${PHONON_INCLUDES})

if(Phonon4Qt${QT_MAJOR_VERSION}_FOUND)
    add_subdirectory(platform_kde)
endif()
