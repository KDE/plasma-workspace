/*
 * Copyright (c) 2001 Reza Arbab <arbab@austin.ibm.com>
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
 */

#include "kcheckpass.h"

#ifdef HAVE_AIX_AUTH
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* 
 * The AIX builtin authenticate() uses whichever method the system 
 * has been configured for.  (/etc/passwd, DCE, etc.)
 */
int authenticate(const char *, const char *, int *, char **);

AuthReturn Authenticate(const char *method,
        const char *login, char *(*conv) (ConvRequest, const char *))
{
  int result;
  int reenter;  /* Tells if authenticate is done processing or not. */
  char *passwd;
  char *msg; /* Contains a prompt message or failure reason.     */

  if (!strcmp(method, "classic")) {

    if (!(passwd = conv(ConvGetHidden, 0)))
      return AuthAbort;

    if ((result = authenticate(login, passwd, &reenter, &msg))) {
      if (msg) {
        conv(ConvPutError, msg);
        free(msg);
      }
      dispose(passwd);
      return AuthBad;
    }
    if (reenter) {
      char buf[256];
      snprintf(buf, sizeof(buf), "More authentication data requested: %s\n", msg);
      conv(ConvPutError, buf);
      free(msg);
      dispose(passwd);
      return result == ENOENT || result == ESAD ? AuthBad : AuthError;
    }
    dispose(passwd);
    return AuthOk;

  } else if (!strcmp(method, "generic")) {

    for (passwd = 0;;) {
      if ((result = authenticate(login, passwd, &reenter, &msg))) {
        if (msg) {
          conv(ConvPutError, msg);
          free(msg);
        }
        if (passwd)
          dispose(passwd);
        return result == ENOENT || result == ESAD ? AuthBad : AuthError;
      }
      if (passwd)
	dispose(passwd);
      if (!reenter)
        break;
      passwd = conv(ConvGetHidden, msg);
      free(msg);
      if (!passwd)
        return AuthAbort;
    }
    return AuthOk;

  } else
    return AuthError;

}

#endif
