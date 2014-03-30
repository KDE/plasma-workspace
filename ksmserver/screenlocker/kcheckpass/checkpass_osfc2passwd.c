/*
 *
 * Copyright (C) 1999 Mark Davies <mark@MCS.VUW.AC.NZ>
 * Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
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

#ifdef HAVE_OSF_C2_PASSWD

static char *osf1c2crypt(const char *pw, char *salt);
static int osf1c2_getprpwent(char *p, char *n, int len);

/*******************************************************************
 * This is the authentication code for OSF C2 security passwords
 *******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

AuthReturn Authenticate(const char *method,
        const char *login, char *(*conv) (ConvRequest, const char *))
{
  char *passwd;
  char *crpt_passwd;
  char c2passwd[256];

  if (strcmp(method, "classic"))
    return AuthError;

  if (!osf1c2_getprpwent(c2passwd, login, sizeof(c2passwd)))
    return AuthBad;

  if (!*c2passwd)
    return AuthOk;

  if (!(passwd = conv(ConvGetHidden, 0)))
    return AuthAbort;

  if ((crpt_passwd = osf1c2crypt(passwd, c2passwd)) && !strcmp(c2passwd, crpt_passwd)) {
    dispose(passwd);
    return AuthOk; /* Success */
  }
  dispose(passwd);
  return AuthBad; /* Password wrong or account locked */
}


/*
The following code was lifted from the file osfc2.c from the ssh 1.2.26
distribution.  Parts of the code that were not needed by kcheckpass
(notably the osf1c2_check_account_and_terminal() function and the code
to set the external variable days_before_password_expires have been
removed).  The original copyright from the osfc2.c file is included
below.
*/

/*

osfc2.c

Author: Christophe Wolfhugel

Copyright (c) 1995 Christophe Wolfhugel

Free use of this file is permitted for any purpose as long as
this copyright is preserved in the header.

This program implements the use of the OSF/1 C2 security extensions
within ssh. See the file COPYING for full licensing information.

*/

#include <sys/security.h>
#include <prot.h>
#include <sia.h>

static int     c2security = -1;
static int     crypt_algo;

static void
initialize_osf_security(int ac, char **av)
{
  FILE *f;
  char buf[256];
  char siad[] = "siad_ses_init=";

  if (access(SIAIGOODFILE, F_OK) == -1)
    {
      /* Broken OSF/1 system, better don't run on it. */
      fprintf(stderr, SIAIGOODFILE);
      fprintf(stderr, " does not exist. Your OSF/1 system is probably broken\n");
      exit(1);
    }
  if ((f = fopen(MATRIX_CONF, "r")) == NULL)
    {
      /* Another way OSF/1 is probably broken. */
      fprintf(stderr, "%s unreadable. Your OSF/1 system is probably broken.\n"

             MATRIX_CONF);
      exit(1);
    }

  /* Read matrix.conf to check if we run C2 or not */
  while (fgets(buf, sizeof(buf), f) != NULL)
    {
      if (strncmp(buf, siad, sizeof(siad) - 1) == 0)
       {
         if (strstr(buf, "OSFC2") != NULL)
           c2security = 1;
         else if (strstr(buf, "BSD") != NULL)
           c2security = 0;
         break;
       }
    }
  fclose(f);
  if (c2security == -1)
    {
      fprintf(stderr, "C2 security initialization failed : could not determine security level.\n");
      exit(1);
    }
  if (c2security == 1)
    set_auth_parameters(ac, av);
}


static int
osf1c2_getprpwent(char *p, char *n, int len)
{
  time_t pschg, tnow;

  if (c2security == 1)
    {
      struct es_passwd *es;
      struct pr_passwd *pr = getprpwnam(n);
      if (pr)
       {
         strlcpy(p, pr->ufld.fd_encrypt, len);
         crypt_algo = pr->ufld.fd_oldcrypt;

         tnow = time(NULL);
         if (pr->uflg.fg_schange == 1)
           pschg = pr->ufld.fd_schange;
         else
           pschg = 0;
         if (pr->uflg.fg_template == 0)
           {
             /** default template, system values **/
             if (pr->sflg.fg_lifetime == 1)
               if (pr->sfld.fd_lifetime > 0 &&
                   pschg + pr->sfld.fd_lifetime < tnow)
                 return 1;
           }
         else                      /** user template, specific values **/
           {
             es = getespwnam(pr->ufld.fd_template);
             if (es)
               {
                 if (es->uflg->fg_expire == 1)
                   if (es->ufld->fd_expire > 0 &&
                       pschg + es->ufld->fd_expire < tnow)
                     return 1;
               }
           }
       }
    }
  else
    {
      struct passwd *pw = getpwnam(n);
      if (pw)
        {
          strlcpy(p, pw->pw_passwd, len);
          return 1;
        }
    }
  return 0;
}

static char *
osf1c2crypt(const char *pw, char *salt)
{
   if (c2security == 1) {
     return(dispcrypt(pw, salt, crypt_algo));
   } else
     return(crypt(pw, salt));
}

#endif
