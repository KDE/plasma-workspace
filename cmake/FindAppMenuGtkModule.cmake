#.rst:
# FindAppmenuGtkModule
# -----------
#
# Try to find appmenu-gtk2-module and appmenu-gtk3-module.
# Once done this will define:
#
# ``AppMenuGtkModule_FOUND``
#     System has both appmenu-gtk2-module and appmenu-gtk3-module

#=============================================================================
# Copyright (c) 2018 Kai Uwe Broulik <kde@broulik.de>
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

find_library(AppMenuGtk2Module_LIBRARY libappmenu-gtk-module.so
    PATH_SUFFIXES
        gtk-2.0/modules
)

find_library(AppMenuGtk3Module_LIBRARY libappmenu-gtk-module.so
    PATH_SUFFIXES
        gtk-3.0/modules
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AppMenuGtkModule
    FOUND_VAR
        AppMenuGtkModule_FOUND
    REQUIRED_VARS
        AppMenuGtk3Module_LIBRARY
        AppMenuGtk2Module_LIBRARY
)

mark_as_advanced(AppMenuGtk3Module_LIBRARY AppMenuGtk2Module_LIBRARY)

include(FeatureSummary)
set_package_properties(AppMenuGtkModule PROPERTIES
    URL "https://github.com/rilian-la-te/vala-panel-appmenu/tree/master/subprojects/appmenu-gtk-module"
    DESCRIPTION "Application Menu GTK+ Module"
)
