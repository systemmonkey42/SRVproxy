# $Id: Makefile,v 1.3 2002/10/01 20:34:28 vanrein Exp $
#
# Makefile for SRVproxyHTTP, a simple but useful SRV-aware proxy.
#
# This tool assumes that libsrv has been installed on the system, and that
# the header file is, too.
#

TARGETS=SRVproxyHTTP

PREFIX=/usr/local

# Under what prefix is libsrv stuff installed?
LIBSRVPREFIX=/usr/local


# Linking command for LIBSRV; possibly prefix with -L option to gcc if needed
LIBSRV=-L$(LIBSRVPREFIX)/lib -lsrv


# Explicit linkage of resolv.conf is needed on most, but not all platforms
#
# Linux, Solaris, OpenBSD:
## RESOLV=-lresolv
# FreeBSD:
RESOLV=

CC=gcc
CCOPTS=-I$(LIBSRVPREFIX)/include
LIBS=$(LIBSRV) $(RESOLV)

all: $(TARGETS)

SRVproxyHTTP: SRVproxyHTTP.c
	$(CC) $(CCOPTS) -o SRVproxyHTTP SRVproxyHTTP.c $(LIBS)

clean:
	rm -f $(TARGETS)

very: clean

anew: clean all

install: all
	install -m 755 SRVproxyHTTP $(PREFIX)/sbin

uninstall:
	rm -f $(PREFIX)/sbin/SRVproxyHTTP


#
# $Log: Makefile,v $
# Revision 1.3  2002/10/01 20:34:28  vanrein
# Separated this module from the libsrv module (kept it in same repository).
# Stripped the host part from absURLs, so servers aren't treated like proxies.
# It seems to work on many Unix systems (Linux, FreeBSD, OpenBSD, MacOS X).
#
#
