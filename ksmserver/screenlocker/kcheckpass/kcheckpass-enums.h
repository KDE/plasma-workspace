/*****************************************************************
 *
 *·     kcheckpass
 *
 *·     Simple password checker. Just invoke and send it
 *·     the password on stdin.
 *
 *·     If the password was accepted, the program exits with 0;
 *·     if it was rejected, it exits with 1. Any other exit
 *·     code signals an error.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *·     Copyright (C) 1998, Caldera, Inc.
 *·     Released under the GNU General Public License
 *
 *·     Olaf Kirch <okir@caldera.de>      General Framework and PAM support
 *·     Christian Esken <esken@kde.org>   Shadow and /etc/passwd support
 *·     Oswald Buddenhagen <ossi@kde.org> Binary server mode
 *
 *      Other parts were taken from kscreensaver's passwd.cpp
 *****************************************************************/

#ifndef KCHECKPASS_ENUMS_H
#define KCHECKPASS_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* these must match kcheckpass' exit codes */
typedef enum {
    AuthOk = 0,
    AuthBad = 1,
    AuthError = 2,
    AuthAbort = 3
} AuthReturn;

typedef enum {
    ConvGetBinary,
    ConvGetNormal,
    ConvGetHidden,
    ConvPutInfo,
    ConvPutError
} ConvRequest;

/* these must match the defs in kgreeterplugin.h */
typedef enum {
    IsUser = 1, /* unused in kcheckpass */
    IsPassword = 2
} DataTag;

#ifdef __cplusplus
}
#endif

#endif /* KCHECKPASS_ENUMS_H */
