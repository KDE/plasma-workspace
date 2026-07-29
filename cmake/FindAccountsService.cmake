# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

find_package(PkgConfig QUIET)
pkg_check_modules(accountsservice QUIET IMPORTED_TARGET GLOBAL accountsservice)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AccountsService
    REQUIRED_VARS
        accountsservice_FOUND
    VERSION_VAR
        accountsservice_VERSION
)

mark_as_advanced(accountsservice_VERSION)
