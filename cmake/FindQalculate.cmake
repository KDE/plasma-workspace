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
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

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

