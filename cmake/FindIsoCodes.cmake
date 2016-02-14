# - Try to find iso-codes
# Once done this will define
#
#  IsoCodes_FOUND - system has iso-codes
#  IsoCodes_DOMAINS - the available domains provided by iso-codes
#
# Copyright (c) 2016, Pino Toscano, <pino@kde.org>
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

find_package(PkgConfig)
if (PkgConfig_FOUND)
  if (IsoCodes_MIN_VERSION)
    pkg_check_modules(_pc_ISOCODES iso-codes>=${IsoCodes_MIN_VERSION})
  else ()
    pkg_check_modules(_pc_ISOCODES iso-codes)
  endif ()
  if (_pc_ISOCODES_FOUND)
    pkg_get_variable(IsoCodes_DOMAINS iso-codes domains)
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(IsoCodes DEFAULT_MSG IsoCodes_DOMAINS)
find_package_handle_standard_args(IsoCodes REQUIRED_VARS _pc_ISOCODES_FOUND IsoCodes_DOMAINS VERSION_VAR _pc_ISOCODES_VERSION)

mark_as_advanced(IsoCodes_DOMAINS)
