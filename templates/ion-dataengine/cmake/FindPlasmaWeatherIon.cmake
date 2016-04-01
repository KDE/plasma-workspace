# - Try to find the Plasma Weather Ion library
# Once done this will define
#
#  PlasmaWeatherIon_FOUND - system has Plasma Weather Ion
#  PlasmaWeatherIon_INCLUDE_DIR - the Plasma Weather Ion include directory
#  PlasmaWeatherIon_LIBRARIES - Plasma Weather Ion library

if (PlasmaWeatherIon_INCLUDE_DIR AND PlasmaWeatherIon_LIBRARY)
    # Already in cache, be silent
    set(PlasmaWeatherIon_FIND_QUIETLY TRUE)
endif (PlasmaWeatherIon_INCLUDE_DIR AND PlasmaWeatherIon_LIBRARY)

find_path(PlasmaWeatherIon_INCLUDE_DIR NAMES plasma/weather/ion.h)
find_library(PlasmaWeatherIon_LIBRARY weather_ion)

if (PlasmaWeatherIon_INCLUDE_DIR AND PlasmaWeatherIon_LIBRARY)
    set(PlasmaWeatherIon_FOUND TRUE)
    set(PlasmaWeatherIon_LIBRARIES ${PlasmaWeatherIon_LIBRARY})
endif (PlasmaWeatherIon_INCLUDE_DIR AND PlasmaWeatherIon_LIBRARY)

if (PlasmaWeatherIon_FOUND)
    if (NOT PlasmaWeatherIon_FIND_QUIETLY)
        message(STATUS "Found Plasma Weather Ion library: ${PlasmaWeatherIon_LIBRARIES}")
    endif (NOT PlasmaWeatherIon_FIND_QUIETLY)
else (PlasmaWeatherIon_FOUND)
    if (PlasmaWeatherIon_FIND_REQUIRED)
        message(FATAL_ERROR "Plasma Weather Ion library was not found")
    endif(PlasmaWeatherIon_FIND_REQUIRED)
endif (PlasmaWeatherIon_FOUND)

mark_as_advanced(PlasmaWeatherIon_INCLUDE_DIR PlasmaWeatherIon_LIBRARY)
