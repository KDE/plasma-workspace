# LIBGPS_FOUND - system has the LIBGPS library
# LIBGPS_INCLUDE_DIR - the LIBGPS include directory
# LIBGPS_LIBRARIES - The libraries needed to use LIBGPS

if(LIBGPS_INCLUDE_DIR AND LIBGPS_LIBRARIES)
  set(LIBGPS_FOUND TRUE)
else(LIBGPS_INCLUDE_DIR AND LIBGPS_LIBRARIES)

  find_path(LIBGPS_INCLUDE_DIR NAMES gps.h)
  find_library(LIBGPS_LIBRARIES NAMES gps)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(libgps DEFAULT_MSG LIBGPS_INCLUDE_DIR LIBGPS_LIBRARIES)

  mark_as_advanced(LIBGPS_INCLUDE_DIR LIBGPS_LIBRARIES)
endif(LIBGPS_INCLUDE_DIR AND LIBGPS_LIBRARIES)
