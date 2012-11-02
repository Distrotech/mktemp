/*
 * Copyright (c) 2001, 2004, 2008, 2010
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

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif /* HAVE_STRINGS_H */
#endif /* HAVE_STRING_H */
#if defined(HAVE_MALLOC_H) && !defined(STDC_HEADERS)
# include <malloc.h>
#endif /* HAVE_MALLOC_H && !STDC_HEADERS */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>

#include <extern.h>

/*
 * Very simple-minded mkdtemp() replacement.
 */
char *
mkdtemp(template)
	char *template;
{
	char *otemplate;
	int i, oerrno, error;

	otemplate = strdup(template);
	if (!otemplate)
		return(NULL);

	for (i = 0; i < 1000; i++) {
		error = mktemp(template) == NULL;
		if (error)
			break;
		error = mkdir(template, S_IRUSR|S_IWUSR|S_IXUSR);
		if (!error || errno != EEXIST)
			break;
		(void)strcpy(template, otemplate); /* SAFE */
	}
	oerrno = errno;
	free(otemplate);
	errno = oerrno;
	return(error ? NULL : template);
}
