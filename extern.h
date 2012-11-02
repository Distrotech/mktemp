/*
 * Copyright (c) 2001, 2008, 2010 Todd C. Miller <Todd.Miller@courtesan.com>
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

#ifndef _MKTEMP_EXTERN_H
#define _MKTEMP_EXTERN_H

#ifndef errno
extern int errno;
#endif

extern char *MKDTEMP __P((char *));
extern int MKSTEMP __P((char *));
#ifndef HAVE_ARC4RANDOM
extern unsigned int arc4random __P((void));
extern void arc4random_stir __P((void));
extern void arc4random_addrandom __P((unsigned char *, int));
extern void arc4random_buf __P((void *, size_t));
#endif
#ifndef HAVE_ARC4RANDOM_UNIFORM
extern unsigned int arc4random_uniform __P((unsigned int));
#endif

#endif /* _MKTEMP_EXTERN_H */
