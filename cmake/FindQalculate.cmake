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
# SPDX-FileCopyrightText: 2007 Vladimir Kuznetsov <ks.vladimir@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause

if(QALCULATE_CFLAGS AND QALCULATE_LIBRARIES)

  # in cache already
  set(QALCULATE_FOUND TRUE)

else(QALCULATE_CFLAGS AND QALCULATE_LIBRARIES)
  if(NOT WIN32)
    find_package(PkgConfig)

    if(QALCULATE_MIN_VERSION)
      pkg_check_modules(_pc_QALCULATE libqalculate>=${QALCULATE_MIN_VERSION})
    else(QALCULATE_MIN_VERSION)
      pkg_check_modules(_pc_QALCULATE libqalculate)
    endif(QALCULATE_MIN_VERSION)

    if(_pc_QALCULATE_FOUND)
      if(${_pc_QALCULATE_VERSION} VERSION_LESS 2.0.0)
        pkg_check_modules(_pc_CLN cln)
      endif()
      set(QALCULATE_CFLAGS ${_pc_QALCULATE_CFLAGS})
    endif()

    find_library(QALCULATE_LIBRARIES
      NAMES
      qalculate
      PATHS
      ${_pc_QALCULATE_LIBRARY_DIRS}
      ${LIB_INSTALL_DIR}
    )

    find_path(QALCULATE_INCLUDE_DIR
      NAMES
      libqalculate
      PATHS
      ${_pc_QALCULATE_INCLUDE_DIRS}
      ${INCLUDE_INSTALL_DIR}
    )

    if(_pc_QALCULATE_FOUND)
      if(${_pc_QALCULATE_VERSION} VERSION_LESS 2.0.0)
        find_library(CLN_LIBRARIES
          NAMES
          cln
          PATHS
          ${_pc_CLN_LIBRARY_DIRS}
          ${LIB_INSTALL_DIR}
        )
      endif()
    endif()

  else(NOT WIN32)
    # XXX: currently no libqalculate on windows
    set(QALCULATE_FOUND FALSE)

  endif(NOT WIN32)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Qalculate DEFAULT_MSG QALCULATE_LIBRARIES )

  mark_as_advanced(QALCULATE_CFLAGS QALCULATE_LIBRARIES)

endif(QALCULATE_CFLAGS AND QALCULATE_LIBRARIES)

