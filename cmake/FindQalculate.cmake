# - Try to find libqalculate
# Input variables
#
#  QALCULATE_MIN_VERSION - minimal version of libqalculate
#  QALCULATE_FIND_REQUIRED - fail if can't find libqalculate
#
# Once done this will define
#
#  QALCULATE_FOUND - system has libqalculate
#  QALCULATE_CFLAGS - libqalculate cflags
#  QALCULATE_LIBRARIES - libqalculate libraries
#
# Copyright (c) 2007, Vladimir Kuznetsov, <ks.vladimir@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(QALCULATE_CFLAGS AND QALCULATE_LIBRARIES)

  # in cache already
  set(QALCULATE_FOUND TRUE)

else(QALCULATE_CFLAGS AND QALCULATE_LIBRARIES)
  if(NOT WIN32)
    include(UsePkgConfig)

    if(QALCULATE_MIN_VERSION)
      exec_program(${PKGCONFIG_EXECUTABLE} ARGS libqalculate --atleast-version=${QALCULATE_MIN_VERSION} RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull)
    else(QALCULATE_MIN_VERSION)
      exec_program(${PKGCONFIG_EXECUTABLE} ARGS libqalculate --exists RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull)
    endif(QALCULATE_MIN_VERSION)

    if(_return_VALUE STREQUAL "0")
      exec_program(${PKGCONFIG_EXECUTABLE} ARGS libqalculate --libs OUTPUT_VARIABLE QALCULATE_LIBRARIES)
      exec_program(${PKGCONFIG_EXECUTABLE} ARGS cln --libs OUTPUT_VARIABLE CLN_LIBRARIES)
      exec_program(${PKGCONFIG_EXECUTABLE} ARGS libqalculate --cflags OUTPUT_VARIABLE QALCULATE_CFLAGS)
      set(QALCULATE_FOUND TRUE)
    endif(_return_VALUE STREQUAL "0")

  else(NOT WIN32)
    # XXX: currently no libqalculate on windows
    set(QALCULATE_FOUND FALSE)

  endif(NOT WIN32)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Qalculate DEFAULT_MSG QALCULATE_LIBRARIES )

  mark_as_advanced(QALCULATE_CFLAGS QALCULATE_LIBRARIES)

endif(QALCULATE_CFLAGS AND QALCULATE_LIBRARIES)

