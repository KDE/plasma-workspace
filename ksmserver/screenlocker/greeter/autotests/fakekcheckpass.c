/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

This test is based on kcheckpass, written by:

Olaf Kirch <okir@caldera.de>         General Framework and PAM support
Christian Esken <esken@kde.org>      Shadow and /etc/passwd support
Roberto Teixeira <maragato@kde.org>  other user (-U) support
Oswald Buddenhagen <ossi@kde.org>    Binary server mode

*********************************************************************/
#include <kcheckpass-enums.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

static int havetty, sfd = -1, nullpass;

#ifdef __cplusplus
extern "C" {
#endif

# define ATTR_PRINTFLIKE(fmt,var) __attribute__((format(printf,fmt,var)))
void message(const char *, ...) ATTR_PRINTFLIKE(1, 2);

#ifdef __cplusplus
}
#endif

static int
Reader (void *buf, int count)
{
    int ret, rlen;

    for (rlen = 0; rlen < count; ) {
      dord:
        ret = read (sfd, (void *)((char *)buf + rlen), count - rlen);
        if (ret < 0) {
            if (errno == EINTR)
                goto dord;
            if (errno == EAGAIN)
                break;
            return -1;
        }
        if (!ret)
            break;
        rlen += ret;
    }
    return rlen;
}

static void
GRead (void *buf, int count)
{
    if (Reader (buf, count) != count) {
        message ("Communication breakdown on read\n");
        exit(15);
    }
}

static void
GWrite (const void *buf, int count)
{
    if (write (sfd, buf, count) != count) {
        message ("Communication breakdown on write\n");
        exit(15);
    }
}

static void
GSendInt (int val)
{
    GWrite (&val, sizeof(val));
}

static void
GSendStr (const char *buf)
{
    unsigned len = buf ? strlen (buf) + 1 : 0;
    GWrite (&len, sizeof(len));
    GWrite (buf, len);
}

static void
GSendArr (int len, const char *buf)
{
    GWrite (&len, sizeof(len));
    GWrite (buf, len);
}

static int
GRecvInt (void)
{
    int val;

    GRead (&val, sizeof(val));
    return val;
}

static char *
GRecvStr (void)
{
    unsigned len;
    char *buf;

    if (!(len = GRecvInt()))
        return (char *)0;
    if (len > 0x1000 || !(buf = malloc (len))) {
        message ("No memory for read buffer\n");
        exit(15);
    }
    GRead (buf, len);
    buf[len - 1] = 0; /* we're setuid ... don't trust "them" */
    return buf;
}

static char *
GRecvArr (void)
{
    unsigned len;
    char *arr;
    unsigned const char *up;

    if (!(len = (unsigned) GRecvInt()))
        return (char *)0;
    if (len < 4) {
        message ("Too short binary authentication data block\n");
        exit(15);
    }
    if (len > 0x10000 || !(arr = malloc (len))) {
        message ("No memory for read buffer\n");
        exit(15);
    }
    GRead (arr, len);
    up = (unsigned const char *)arr;
    if (len != (unsigned)(up[3] | (up[2] << 8) | (up[1] << 16) | (up[0] << 24))) {
        message ("Mismatched binary authentication data block size\n");
        exit(15);
    }
    return arr;
}


static char *
conv_server (ConvRequest what, const char *prompt)
{
    GSendInt (what);
    switch (what) {
    case ConvGetBinary:
      {
        unsigned const char *up = (unsigned const char *)prompt;
        int len = up[3] | (up[2] << 8) | (up[1] << 16) | (up[0] << 24);
        GSendArr (len, prompt);
        return GRecvArr ();
      }
    case ConvGetNormal:
    case ConvGetHidden:
      {
        char *msg;
        GSendStr (prompt);
        msg = GRecvStr ();
        if (msg && (GRecvInt() & IsPassword) && !*msg)
          nullpass = 1;
        return msg;
      }
    case ConvPutInfo:
    case ConvPutError:
    default:
        GSendStr (prompt);
        return 0;
    }
}

void
message(const char *fmt, ...)
{
  va_list               ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

int
main(int argc, char **argv)
{
#ifdef HAVE_PAM
  const char    *caller = KSCREENSAVER_PAM_SERVICE;
#endif
  const char    *method = "classic";
  const char    *username = 0;
#ifdef ACCEPT_ENV
  char          *p;
#endif
  int           c, nfd;

#ifdef HAVE_OSF_C2_PASSWD
  initialize_osf_security(argc, argv);
#endif

  /* Make sure stdout/stderr are open */
  for (c = 1; c <= 2; c++) {
    if (fcntl(c, F_GETFL) == -1) {
      if ((nfd = open("/dev/null", O_WRONLY)) < 0) {
        message("cannot open /dev/null: %s\n", strerror(errno));
        exit(10);
      }
      if (c != nfd) {
        dup2(nfd, c);
        close(nfd);
      }
    }
  }

  havetty = isatty(0);

  while ((c = getopt(argc, argv, "hc:m:U:S:")) != -1) {
    switch (c) {
    case 'c':
#ifdef HAVE_PAM
      caller = optarg;
#endif
      break;
    case 'm':
      method = optarg;
      break;
    case 'U':
      username = optarg;
      break;
    case 'S':
      sfd = atoi(optarg);
      break;
    default:
      message("Command line option parsing error\n");
    }
  }

#ifdef ACCEPT_ENV
# ifdef HAVE_PAM
  if ((p = getenv("KDE_PAM_ACTION")))
    caller = p;
# endif
  if ((p = getenv("KCHECKPASS_USER")))
    username = p;
#endif


  /* Now do the fandango */
  const char *password = conv_server(ConvGetHidden, 0);
  if (strcmp(password, "testpassword") == 0) {
      return AuthOk;
  } else if (strcmp(password, "info") == 0) {
      conv_server(ConvPutInfo, "You wanted some info, here you have it");
      return AuthOk;
  } else if (strcmp(password, "error") == 0) {
      conv_server(ConvPutError, "The world is going to explode");
      return AuthError;
  } else if (strcmp(password, "") == 0) {
      conv_server(ConvPutError, "Hey, don't give me an empty password");
      return AuthError;
  }
  return AuthBad;
}
