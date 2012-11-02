/*	$OpenBSD: mktemp.c,v 1.13 2003/06/17 21:56:25 millert Exp $	*/

/*
 * Copyright (c) 1996, 1997, 2001, 2004, 2008, 2010
 *	Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif /* HAVE_STRINGS_H */
#endif /* HAVE_STRING_H */
#if defined(HAVE_MALLOC_H) && !defined(STDC_HEADERS)
#include <malloc.h>
#endif /* HAVE_MALLOC_H && !STDC_HEADERS */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif /* HAVE_PATHS_H */
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif /* HAVE_GETOPT_LONG */
#include <errno.h>

#include <extern.h>

#ifndef _PATH_TMP
#define _PATH_TMP "/tmp"
#endif

#ifdef HAVE_PROGNAME
extern char *__progname;
#else
char *__progname;
#endif

void usage __P((void)) __attribute__((__noreturn__));

#ifdef HAVE_GETOPT_LONG
static struct option const longopts[] =
{
  {"directory",	no_argument,		NULL,	'd'},
  {"help",	no_argument,		NULL,	'h'},
  {"quiet",	no_argument,		NULL,	'q'},
  {"tmpdir",	optional_argument,	NULL,	'T'},
  {"dry-run",	no_argument,		NULL,	'u'},
  {"version",	no_argument,		NULL,	'V'},
  {NULL,	0,			NULL,	0}
};
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch, fd, uflag = 0, quiet = 0, tflag = 0, Tflag = 0, makedir = 0;
	char *cp, *template, *tempfile, *prefix = _PATH_TMP;
	size_t plen;
	extern char *optarg;
	extern int optind;

#ifndef HAVE_PROGNAME
	__progname = argv[0];
#endif

#ifdef HAVE_GETOPT_LONG
	while ((ch = getopt_long(argc, argv, "dp:qtuV", longopts, NULL)) != -1)
#else
	while ((ch = getopt(argc, argv, "dp:qtuV")) != -1)
#endif
		switch (ch) {
		case 'd':
			makedir = 1;
			break;
		case 'p':
			prefix = optarg;
			tflag = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'T':
			if (optarg) {
				Tflag = 1;
				prefix = optarg;
			}
			/* FALLTHROUGH */
		case 't':
			tflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'V':
			printf("%s version %s\n", __progname, PACKAGE_VERSION);
			exit(0);
		default:
			usage();
	}

	/* If no template specified use a default one (implies -t mode) */
	switch (argc - optind) {
	case 1:
		template = argv[optind];
		break;
	case 0:
		template = "tmp.XXXXXXXXXX";
		tflag = 1;
		break;
	default:
		usage();
	}

	if (tflag) {
		if (strchr(template, '/')) {
			if (!quiet)
				(void)fprintf(stderr,
				    "%s: template must not contain directory separators in -t mode\n", __progname);
			exit(1);
		}

		if (!Tflag) {
			cp = getenv("TMPDIR");
			if (cp != NULL && *cp != '\0')
				prefix = cp;
		}
		plen = strlen(prefix);
		while (plen != 0 && prefix[plen - 1] == '/')
			plen--;

		tempfile = (char *)malloc(plen + 1 + strlen(template) + 1);
		if (tempfile == NULL) {
			if (!quiet)
				(void)fprintf(stderr,
				    "%s: cannot allocate memory\n", __progname);
			exit(1);
		}
		(void)memcpy(tempfile, prefix, plen);
		tempfile[plen] = '/';
		(void)strcpy(tempfile + plen + 1, template);	/* SAFE */
	} else {
		if ((tempfile = strdup(template)) == NULL) {
			if (!quiet)
				(void)fprintf(stderr,
				    "%s: cannot allocate memory\n", __progname);
			exit(1);
		}
	}

	if (makedir) {
		if (MKDTEMP(tempfile) == NULL) {
			if (!quiet) {
				(void)fprintf(stderr,
				    "%s: cannot make temp dir %s: %s\n",
				    __progname, tempfile, strerror(errno));
			}
			exit(1);
		}

		if (uflag)
			(void)rmdir(tempfile);
	} else {
		if ((fd = MKSTEMP(tempfile)) < 0) {
			if (!quiet) {
				(void)fprintf(stderr,
				    "%s: cannot create temp file %s: %s\n",
				    __progname, tempfile, strerror(errno));
			}
			exit(1);
		}
		(void)close(fd);

		if (uflag)
			(void)unlink(tempfile);
	}

	(void)puts(tempfile);
	free(tempfile);

	exit(0);
}

void
usage()
{

	(void)fprintf(stderr,
	    "Usage: %s [-V] | [-dqtu] [-p prefix] [template]\n",
	    __progname);
	exit(1);
}
