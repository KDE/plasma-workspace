/*****************************************************************
 *
 *	kcheckpass - Simple password checker
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
 *
 *	kcheckpass is a simple password checker. Just invoke and
 *      send it the password on stdin.
 *
 *	If the password was accepted, the program exits with 0;
 *	if it was rejected, it exits with 1. Any other exit
 *	code signals an error.
 *
 *	It's hopefully simple enough to allow it to be setuid
 *	root.
 *
 *	Compile with -DHAVE_VSYSLOG if you have vsyslog().
 *	Compile with -DHAVE_PAM if you have a PAM system,
 *	and link with -lpam -ldl.
 *	Compile with -DHAVE_SHADOW if you have a shadow
 *	password system.
 *
 *	Copyright (C) 1998, Caldera, Inc.
 *	Released under the GNU General Public License
 *
 *	Olaf Kirch <okir@caldera.de>         General Framework and PAM support
 *	Christian Esken <esken@kde.org>      Shadow and /etc/passwd support
 *	Roberto Teixeira <maragato@kde.org>  other user (-U) support
 *	Oswald Buddenhagen <ossi@kde.org>    Binary server mode
 *
 *      Other parts were taken from kscreensaver's passwd.cpp.
 *
 *****************************************************************/

#include "kcheckpass.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

/* Compatibility: accept some options from environment variables */
#define ACCEPT_ENV

#define THROTTLE 3

static int havetty, sfd = -1, nullpass;

static char *
conv_legacy (ConvRequest what, const char *prompt)
{
    char		*p, *p2;
    int			len;
    char		buf[1024];

    switch (what) {
    case ConvGetBinary:
	break;
    case ConvGetNormal:
	/* there is no prompt == 0 case */
	if (!havetty)
	    break;
	/* i guess we should use /dev/tty ... */
	fputs(prompt, stdout);
	fflush(stdout);
	if (!fgets(buf, sizeof(buf), stdin))
	    return 0;
	len = strlen(buf);
	if (len && buf[len - 1] == '\n')
	    buf[--len] = 0;
	return strdup(buf);
    case ConvGetHidden:
	if (havetty) {
#ifdef HAVE_GETPASSPHRASE
	    p = getpassphrase(prompt ? prompt : "Password: ");
#else
	    p = getpass(prompt ? prompt : "Password: ");
#endif
	    p2 = strdup(p);
	    memset(p, 0, strlen(p));
	    return p2;
	} else {
	    if (prompt)
		break;
	    if ((len = read(0, buf, sizeof(buf) - 1)) < 0) {
		message("Cannot read password\n");
		return 0;
	    } else {
		if (len && buf[len - 1] == '\n')
		    --len;
		buf[len] = 0;
		p2 = strdup(buf);
		memset(buf, 0, len);
		return p2;
	    }
	}
    case ConvPutInfo:
	message("Information: %s\n", prompt);
	return 0;
    case ConvPutError:
	message("Error: %s\n", prompt);
	return 0;
    }
    message("Authentication backend requested data type which cannot be handled.\n");
    return 0;
}


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
  va_list		ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

static void ATTR_NORETURN
usage(int exitval)
{
  message(
	  "usage: kcheckpass {-h|[-c caller] [-m method] [-U username|-S handle]}\n"
	  "  options:\n"
	  "    -h           this help message\n"
	  "    -U username  authenticate the specified user instead of current user\n"
	  "    -S handle    operate in binary server mode on file descriptor handle\n"
	  "    -c caller    the calling application, effectively the PAM service basename\n"
	  "    -m method    use the specified authentication method (default: \"classic\")\n"
	  "  exit codes:\n"
	  "    0 success\n"
	  "    1 invalid password\n"
	  "    2 cannot read password database\n"
	  "    Anything else tells you something's badly hosed.\n"
	);
  exit(exitval);
}

int
main(int argc, char **argv)
{
#ifdef HAVE_PAM
  const char	*caller = KSCREENSAVER_PAM_SERVICE;
#endif
  const char	*method = "classic";
  const char	*username = 0;
#ifdef ACCEPT_ENV
  char		*p;
#endif
  struct passwd	*pw;
  int		c, nfd, lfd;
  uid_t		uid;
  time_t	nexttime;
  AuthReturn	ret;
  struct flock lk;
  char fname[64], fcont[64];

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
    case 'h':
      usage(0);
      break;
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
      usage(10);
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

  uid = getuid();
  if (!username) {
    if (!(p = getenv("LOGNAME")) || !(pw = getpwnam(p)) || pw->pw_uid != uid)
      if (!(p = getenv("USER")) || !(pw = getpwnam(p)) || pw->pw_uid != uid)
        if (!(pw = getpwuid(uid))) {
          message("Cannot determinate current user\n");
          return AuthError;
        }
    if (!(username = strdup(pw->pw_name))) {
      message("Out of memory\n");
      return AuthError;
    }
  }

  /*
   * Throttle kcheckpass invocations to avoid abusing it for bruteforcing
   * the password. This delay belongs to the *previous* invocation, where
   * we can't enforce it reliably (without risking giving away the result
   * before it is due). We don't differentiate between success and failure -
   * it's not expected to have a noticeable adverse effect.
   */
  if ( uid != geteuid() ) {
    sprintf(fname, "/var/run/kcheckpass.%d", uid);
    if ((lfd = open(fname, O_RDWR | O_CREAT | O_NOFOLLOW, 0600)) < 0) {
      message("Cannot open lockfile\n");
      return AuthError;
    }

    lk.l_type = F_WRLCK;
    lk.l_whence = SEEK_SET;
    lk.l_start = lk.l_len = 0;
    if (fcntl(lfd, F_SETLKW, &lk)) {
      message("Cannot obtain lock\n");
      return AuthError;
    }

    if ((c = read(lfd, fcont, sizeof(fcont)-1)) > 0 &&
        (fcont[c] = '\0', sscanf(fcont, "%ld", &nexttime) == 1))
    {
      time_t ct = time(0);
      if (nexttime > ct && nexttime < ct + THROTTLE)
        sleep(nexttime - ct);
    }

    lseek(lfd, 0, SEEK_SET);
    write(lfd, fcont, sprintf(fcont, "%lu\n", time(0) + THROTTLE));

    close(lfd);
  }

  /* Now do the fandango */
  ret = Authenticate(
#ifdef HAVE_PAM
                     caller,
#endif
                     method,
                     username, 
                     sfd < 0 ? conv_legacy : conv_server);

    if (ret == AuthBad) {
      message("Authentication failure\n");
      if (!nullpass) {
        openlog("kcheckpass", LOG_PID, LOG_AUTH);
        syslog(LOG_NOTICE, "Authentication failure for %s (invoked by uid %d)", username, uid);
      }
    }

  return ret;
}

void
dispose(char *str)
{
    memset(str, 0, strlen(str));
    free(str);
}

/*****************************************************************
  The real authentication methods are in separate source files.
  Look in checkpass_*.c
*****************************************************************/
