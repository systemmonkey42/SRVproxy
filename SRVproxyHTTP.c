/* SRVproxyHTTP.c -- An SRV proxy for HTTP traffic.
 *
 * This service listens on service _httpproxy._tcp.somedomain on all interfaces,
 * offering HTTP proxy service.  The somedomain value is the first parameter
 * to this program. When contacted with a URL to connect to, it will
 * setup a connection to a web server, based on SRV records when these
 * are available, or using plain A records otherwise.
 *
 * Efforts are made to find a connection if one is possible.  For example,
 * even in the fallback scenario all IP addresses returned by the DNS will
 * be tried until one accepts the connection.
 *
 * This program uses Rick van Rein's libsrv, and cannot be efficient where that
 * library is not.  This practically means that it will not perform well as
 * a server to large networks, but it is quite suitable to run on every
 * person's desktop machine.  The design is minimal, with that particular
 * application in mind.
 *
 * From: Rick van Rein <rick@vanrein.org>
 */


#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <arpa/srvname.h>


#define LINELEN 4096


int main (int argc, char *argv []) {
	int insox, outsox;
	char line [LINELEN+1];
	int urlpos, domnmpos, slashpos;
	char slash;
	size_t linelen;
	fd_set conn;
	int busy;
	int maxfd;

	if (argc != 2) {
		write (2, "Usage: SRVproxyHTTP my.local.dom\n", 33);
	}

	openlog ("httpproxy", LOG_CONS, LOG_DAEMON);
	syslog (LOG_INFO, "Starting HTTP proxy for SRV record support");

	// Now fork off a new server for all requests
	insox = insrv_server ("httpproxy", "tcp", argv [1], NULL, 0, NULL);

	close (0);
	close (1);
	close (2);

	if (insox < 0) {
		syslog (LOG_WARNING,
				"Failed to connect SRVproxyHTTP server: %s",
				strerror (-errno));
		insrv_exit (1);
	}

	linelen = read (insox, line, LINELEN);
	if (linelen == -1) {
		syslog (LOG_ERR, "Failed to read from client");
		insrv_exit (1);
	}
	line [linelen] = 0;
	if (!strncmp (line, "GET ", 4)) {
		urlpos = 4;
	} else if (!strncmp (line, "HEAD ", 5)) {
		urlpos = 5;
	} else if (!strncmp (line, "POST ", 5)) {
		urlpos = 5;
	} else {
		syslog (LOG_ERR, "Parsing error: expected GET/HEAD/POST");
		insrv_exit (1);
	}
	if (strncmp (line+urlpos, "http://", 7)) {
		perror ("Parsing error: expected http://");
		insrv_exit (1);
	}

	slashpos = domnmpos = urlpos+7;
	while (line [slashpos] && (line [slashpos] != '/')
			       && (line [slashpos] != ' ')) {
		slashpos++;
	}
	slash = line [slashpos];	// For later recovery
	line [slashpos] = 0;		// Temporary overwrite

	outsox = insrv_client ("http", "tcp", line+domnmpos, NULL, 0, NULL);
	if (outsox < 0) {
		syslog (LOG_WARNING, "Failed to connect to %s", line+domnmpos);
		insrv_exit (1);
	}

	line [slashpos] = slash;	// Recover

	// Now reduce the absolute URL in the line to a local filename
	memmove (line+urlpos, line+slashpos, linelen - slashpos);
	linelen = linelen - (slashpos - urlpos);

	// Pass the (now reduced) line on to the web server
	write (outsox, line, linelen);

	FD_ZERO (&conn);
	maxfd = (insox > outsox) ? insox : outsox;

	busy = 1;
	while (busy) {
		FD_SET (insox, &conn);
		FD_SET (outsox, &conn);
		if (select (maxfd+1, &conn, NULL, NULL, NULL) == -1) {
			busy = 0;
		} else {
			if (FD_ISSET (insox, &conn)) {
				linelen = read (insox, line, LINELEN);
				if (linelen <= 0) {
					busy = 0;
				} else {
					write (outsox, line, linelen);
					// TODO: ignoreing errors...
				}
			}
			if (FD_ISSET (outsox, &conn)) {
				linelen = read (outsox, line, LINELEN);
				if (linelen <= 0) {
					busy = 0;
				} else {
					write (insox, line, linelen);
					// TODO: ignoreing errors...
				}
			}
		}
	}

	close (outsox);
	close (insox);

	insrv_exit (0);
}
