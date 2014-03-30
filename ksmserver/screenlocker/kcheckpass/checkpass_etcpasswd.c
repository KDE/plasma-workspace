/*
 * Copyright (c) 1998 Christian Esken <esken@kde.org> 
 * Copyright (c) 2003 Oswald Buddenhagen <ossi@kde.org>
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
 *      Copyright (C) 1998, Christian Esken <esken@kde.org>
 */

#include "kcheckpass.h"

#ifdef HAVE_ETCPASSWD

/*******************************************************************
 * This is the authentication code for /etc/passwd passwords
 *******************************************************************/

#include <string.h>
#include <stdlib.h>

AuthReturn Authenticate(const char *method,
        const char *login, char *(*conv) (ConvRequest, const char *))
{
  struct passwd *pw;
  char *passwd;
  char *crpt_passwd;

  if (strcmp(method, "classic"))
    return AuthError;

  /* Get the password entry for the user we want */
  if (!(pw = getpwnam(login)))
    return AuthBad;

  if (!*pw->pw_passwd)
    return AuthOk;

  if (!(passwd = conv(ConvGetHidden, 0)))
    return AuthAbort;

  if ((crpt_passwd = crypt(passwd, pw->pw_passwd)) && !strcmp(pw->pw_passwd, crpt_passwd)) {
    dispose(passwd);
    return AuthOk; /* Success */
  }
  dispose(passwd);
  return AuthBad; /* Password wrong or account locked */
}

#endif
