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
# SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
#
# SPDX-License-Identifier: BSD-3-Clause

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
