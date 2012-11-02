/*	$OpenBSD: arc4random.c,v 1.19 2008/06/04 00:50:23 djm Exp $	*/

/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2008, 2010, Todd C. Miller <Todd.Miller@courtesan.com>
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

/*
 * Arc4 random number generator for OpenBSD.
 *
 * This code is derived from section 17.1 of Applied Cryptography,
 * second edition, which describes a stream cipher allegedly
 * compatible with RSA Labs "RC4" cipher (the actual description of
 * which is a trade secret).  The same algorithm is used as a stream
 * cipher called "arcfour" in Tatu Ylonen's ssh package.
 *
 * Here the stream cipher has been modified always to include the time
 * when initializing the state.  That makes it impossible to
 * regenerate the same random sequence twice, so this can't be used
 * for encryption, but will generate good random numbers.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

#include "config.h"

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_PRNGD
# include <sys/socket.h>
# include <sys/un.h>
# include <netinet/in.h>
#endif /* HAVE_PRNGD */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if defined(TIME_WITH_SYS_TIME) || !defined(HAVE_SYS_TIME_H)
# include <time.h>
#endif

#include <extern.h>

#ifdef __GNUC__
#define inline __inline
#else				/* !__GNUC__ */
#define inline
#endif				/* !__GNUC__ */

#define _ARC4_LOCK()
#define _ARC4_UNLOCK()

#ifndef HAVE_ARC4RANDOM
struct arc4_stream {
	unsigned char i;
	unsigned char j;
	unsigned char s[256];
};

static int rs_initialized;
static struct arc4_stream rs;
static int arc4_count;

static inline unsigned char arc4_getbyte __P((void));

static inline void
arc4_init()
{
	int     n;

	for (n = 0; n < 256; n++)
		rs.s[n] = n;
	rs.i = 0;
	rs.j = 0;
}

static inline void
arc4_addrandom(dat, datlen)
	unsigned char *dat;
	int datlen;
{
	int           n;
	unsigned char si;

	rs.i--;
	for (n = 0; n < 256; n++) {
		rs.i = (rs.i + 1);
		si = rs.s[rs.i];
		rs.j = (rs.j + si + dat[n % datlen]);
		rs.s[rs.i] = rs.s[rs.j];
		rs.s[rs.j] = si;
	}
	rs.j = rs.i;
}

#ifdef HAVE_PRNGD

#ifndef INADDR_LOOPBACK
# define INADDR_LOOPBACK ((unsigned int)0x7f000001)
#endif

static int
open_prngd(len)
	int len;
{
	int fd = -1, salen;
	char buf[2];
	union {
		struct sockaddr_in in;
		struct sockaddr_un un;
	} sa_un;
	struct sockaddr *sa = (struct sockaddr *)&sa_un;

	memset(&sa_un, 0, sizeof(sa_un));
#ifdef PRNGD_PORT
	sa_un.in.sin_family = AF_INET;
	sa_un.in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa_un.in.sin_port = htons(PRNGD_PORT);
	salen = sizeof(sa_un.in);
#else /* PRNGD_PATH */
	sa_un.un.sun_family = AF_UNIX;
	strncpy(sa_un.un.sun_path, PRNGD_PATH, sizeof(sa_un.un.sun_path));
	salen = sizeof(sa_un.un) -
	    sizeof(sa_un.un.sun_path) + strlen(sa_un.un.sun_path);
#endif
	if ((fd = socket(sa->sa_family, SOCK_STREAM, 0)) == -1)
		goto bad;
	if (connect(fd, sa, salen) == -1)
		goto bad;
	signal(SIGPIPE, SIG_IGN);

	buf[0] = 0x02;
	buf[1] = len;
	if (write(fd, buf, 2) != 2)
		goto bad;

	return(fd);
bad:
	if (fd != -1)
		close(fd);
	return(-1);
}
#endif /* HAVE_PRNGD */

static void
arc4_seed()
{
	struct timeval tv;
	int seed[4];
#if defined(_PATH_RANDOM) || defined(HAVE_PRNGD)
	ssize_t nread, offset = 0;
	unsigned char rnd[128];
        int fd = -1;

#ifdef _PATH_RANDOM
        fd = open(_PATH_RANDOM, O_RDONLY);
#endif
#ifdef HAVE_PRNGD
	if (fd == -1)
		fd = open_prngd(sizeof(rnd));
#endif
        if (fd != -1) {
		for (;;) {
			nread = read(fd, rnd + offset, sizeof(rnd) - offset);
			if (nread == -1) {
				if (errno == EINTR || errno == EAGAIN)
					continue; /* XXX - poll on EAGAIN */
				break;
			}
			offset += nread;
			if (offset == sizeof(rnd)) {
				arc4_addrandom(rnd, sizeof(rnd));
				return;
			}
		}
        }
#endif /* _PATH_RANDOM || HAVE_PRNGD */
        /* Don't have /dev/urandom, do our best... */
        gettimeofday(&tv, NULL);

        /* Use time of day and process id multiplied by small primes */
	seed[0] = (getpid() % 1000) * 983;
	seed[1] = tv.tv_usec * 13;
	seed[2] = (tv.tv_sec % 10000) * 523;
	seed[3] = (tv.tv_sec >> 10) * 389;

	arc4_addrandom((unsigned char *)seed, sizeof(seed));
}

static void
arc4_stir()
{
	int     i;

	if (!rs_initialized) {
		arc4_init();
		rs_initialized = 1;
	}

	arc4_seed();

	/*
	 * Discard early keystream, as per recommendations in:
	 * http://www.wisdom.weizmann.ac.il/~itsik/RC4/Papers/Rc4_ksa.ps
	 */
	for (i = 0; i < 256; i++)
		(void)arc4_getbyte();
	arc4_count = 1600000;
}

static inline unsigned char
arc4_getbyte()
{
	unsigned char si, sj;

	rs.i = (rs.i + 1);
	si = rs.s[rs.i];
	rs.j = (rs.j + si);
	sj = rs.s[rs.j];
	rs.s[rs.i] = sj;
	rs.s[rs.j] = si;
	return (rs.s[(si + sj) & 0xff]);
}

unsigned char
__arc4_getbyte()
{
	unsigned char val;

	_ARC4_LOCK();
	if (--arc4_count == 0 || !rs_initialized)
		arc4_stir();
	val = arc4_getbyte();
	_ARC4_UNLOCK();
	return val;
}

static inline unsigned int
arc4_getword()
{
	unsigned int val;
	val = arc4_getbyte() << 24;
	val |= arc4_getbyte() << 16;
	val |= arc4_getbyte() << 8;
	val |= arc4_getbyte();
	return val;
}

void
arc4random_stir()
{
	_ARC4_LOCK();
	arc4_stir();
	_ARC4_UNLOCK();
}

void
arc4random_addrandom(dat, datlen)
	unsigned char *dat;
	int datlen;
{
	_ARC4_LOCK();
	if (!rs_initialized)
		arc4_stir();
	arc4_addrandom(dat, datlen);
	_ARC4_UNLOCK();
}

unsigned int
arc4random()
{
	unsigned int val;
	_ARC4_LOCK();
	arc4_count -= 4;
	if (arc4_count <= 0 || !rs_initialized)
		arc4_stir();
	val = arc4_getword();
	_ARC4_UNLOCK();
	return val;
}

void
arc4random_buf(_buf, n)
	void *_buf;
	size_t n;
{
	unsigned char *buf = (unsigned char *)_buf;
	_ARC4_LOCK();
	if (!rs_initialized)
		arc4_stir();
	while (n--) {
		if (--arc4_count <= 0)
			arc4_stir();
		buf[n] = arc4_getbyte();
	}
	_ARC4_UNLOCK();
}
#endif /* HAVE_ARC4RANDOM */

#ifndef HAVE_ARC4RANDOM_UNIFORM
/*
 * Calculate a uniformly distributed random number less than upper_bound
 * avoiding "modulo bias".
 *
 * Uniformity is achieved by generating new random numbers until the one
 * returned is outside the range [0, 2**32 % upper_bound).  This
 * guarantees the selected random number will be inside
 * [2**32 % upper_bound, 2**32) which maps back to [0, upper_bound)
 * after reduction modulo upper_bound.
 */
unsigned int
arc4random_uniform(upper_bound)
	unsigned int upper_bound;
{
	unsigned int r, min;

	if (upper_bound < 2)
		return 0;

#if defined(ULONG_MAX) && (ULONG_MAX > 0xffffffffUL)
	min = 0x100000000UL % upper_bound;
#else
	/* Calculate (2**32 % upper_bound) avoiding 64-bit math */
	if (upper_bound > 0x80000000)
		min = 1 + ~upper_bound;		/* 2**32 - upper_bound */
	else {
		/* (2**32 - (x * 2)) % x == 2**32 % x when x <= 2**31 */
		min = ((0xffffffff - (upper_bound * 2)) + 1) % upper_bound;
	}
#endif

	/*
	 * This could theoretically loop forever but each retry has
	 * p > 0.5 (worst case, usually far better) of selecting a
	 * number inside the range we need, so it should rarely need
	 * to re-roll.
	 */
	for (;;) {
		r = arc4random();
		if (r >= min)
			break;
	}

	return r % upper_bound;
}
#endif /* HAVE_ARC4RANDOM_UNIFORM */

#if 0
/*-------- Test code for i386 --------*/
#include <stdio.h>
#include <machine/pctr.h>
int
main(int argc, char **argv)
{
	const int iter = 1000000;
	int     i;
	pctrval v;

	v = rdtsc();
	for (i = 0; i < iter; i++)
		arc4random();
	v = rdtsc() - v;
	v /= iter;

	printf("%qd cycles\n", v);
}
#endif
