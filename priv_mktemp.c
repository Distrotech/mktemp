/*
 * Copyright (c) 2001, 2003, 2004, 2008-2010
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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#include <ctype.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <extern.h>

#define MKTEMP_FILE	1
#define MKTEMP_DIR	2

#define TEMPCHARS	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#define NUM_CHARS	(sizeof(TEMPCHARS) - 1)

#ifdef WIN32
# define mkdir(_path, _mode)	_mkdir(_path)
#endif

#ifndef INT_MAX
#define INT_MAX	0x7fffffff
#endif

static int
mktemp_internal(path, mode)
	char *path;
	int mode;
{
	char *start, *cp;
	const char *tempchars = TEMPCHARS;
	unsigned int r, tries;
	int fd;

	if (*path == '\0') {
		errno = EINVAL;
		return (-1);
	}

	for (start = path; *start; start++)
		;
	tries = 1;
	for (; start > path && start[-1] == 'X'; start--) {
		if (tries < INT_MAX / NUM_CHARS)
			tries *= NUM_CHARS;
	}
	tries *= 2;

	do {
		for (cp = start; *cp; cp++) {
			r = arc4random_uniform(NUM_CHARS);
			*cp = tempchars[r];
		}

		switch (mode) {
		case MKTEMP_FILE:
			fd = open(path, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
			if (fd != -1 || errno != EEXIST)
				return (fd);
			break;
		case MKTEMP_DIR:
			if (mkdir(path, S_IRUSR|S_IWUSR|S_IXUSR) == 0)
				return (0);
			if (errno != EEXIST)
				return (-1);
			break;
		}
	} while (--tries);

	errno = EEXIST;
	return (-1);
}

int
priv_mkstemp(path)
	char *path;
{
	return (mktemp_internal(path, MKTEMP_FILE));
}

char *
priv_mkdtemp(path)
	char *path;
{
	int error;

	error = mktemp_internal(path, MKTEMP_DIR);
	return (error ? NULL : path);
}
