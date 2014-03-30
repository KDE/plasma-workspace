/*
 * Copyright (C) 1998 Christian Esken <esken@kde.org>
 * Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
 *
 * This is a modified version of checkpass_shadow.cpp
 *
 * Modifications made by Thorsten Kukuk <kukuk@suse.de>
 *                       Mathias Kettner <kettner@suse.de>
 *
 * ------------------------------------------------------------
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
 */

#include "kcheckpass.h"

/*******************************************************************
 * This is the authentication code for Shadow-Passwords
 *******************************************************************/

#ifdef HAVE_SHADOW
#include <string.h>
#include <stdlib.h>
#include <pwd.h>

#ifndef __hpux
#include <shadow.h>
#endif

AuthReturn Authenticate(const char *method,
        const char *login, char *(*conv) (ConvRequest, const char *))
{
  char          *typed_in_password;
  char          *crpt_passwd;
  char          *password;
  struct passwd *pw;
  struct spwd   *spw;

  if (strcmp(method, "classic"))
    return AuthError;

  if (!(pw = getpwnam(login)))
    return AuthAbort;

  spw = getspnam(login);
  password = spw ? spw->sp_pwdp : pw->pw_passwd;

  if (!*password)
    return AuthOk;

  if (!(typed_in_password = conv(ConvGetHidden, 0)))
    return AuthAbort;

#if defined( __linux__ ) && defined( HAVE_PW_ENCRYPT )
  crpt_passwd = pw_encrypt(typed_in_password, password);  /* (1) */
#else  
  crpt_passwd = crypt(typed_in_password, password);
#endif

  if (crpt_passwd && !strcmp(password, crpt_passwd )) {
    dispose(typed_in_password);
    return AuthOk; /* Success */
  }
  dispose(typed_in_password);
  return AuthBad; /* Password wrong or account locked */
}

/*
 (1) Deprecated - long passwords have known weaknesses.  Also,
     pw_encrypt is non-standard (requires libshadow.a) while
     everything else you need to support shadow passwords is in
     the standard (ELF) libc.
 */
#endif
