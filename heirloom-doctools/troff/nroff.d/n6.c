/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "n6.c	1.12	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)n6.c	1.4 (gritter) 8/8/05
 */

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */

#include "tdef.h"
#include "tw.h"
#include "proto.h"
#include "ext.h"
#include <ctype.h>

/*
 * n6.c -- width functions, sizes and fonts
*/

int	bdtab[NFONT+1] ={ 0, 0, 0, 3, 3, 0, };
int	sbold = 0;
int	fontlab[NFONT+1] = { 0, 'R', 'I', 'B', PAIR('B','I'), 'S', 0 };

extern	int	nchtab;

int 
width(register tchar j)
{
	register int i, k;

	if (j & (ZBIT|MOT)) {
		if (iszbit(j))
			return(0);
		if (isvmot(j))
			return(0);
		k = absmot(j);
		if (isnmot(j))
			k = -k;
		return(k);
	}
	i = cbits(j);
	if (i < ' ') {
		if (i == '\b')
			return(-widthp);
		if (i == PRESC)
			i = eschar;
		else if (iscontrol(i))
			return(0);
	}
	if (i==ohc)
		return(0);
#ifdef EUC
#ifdef NROFF
	if (multi_locale) {
		if ((j & MBMASK) || (j & CSMASK)) {
			switch(j & MBMASK) {
				case BYTE_CHR:
				case LASTOFMB:
					k = t.Char * csi_width[cs(j)];
					break;
				default:
					k = 0;
					break;
			}
			widthp = k;
			return(k);
		}
	}
	i &= 0x1ff;
#endif /* NROFF */
#endif /* EUC */
	i = trtab[i];
	if (i < 32)
		return(0);
	k = t.width[i] * t.Char;
	widthp = k;
	return(k);
}


tchar 
setch(void)
{
	register int j;
	char	temp[10];
	register char	*s;

	s = temp;
	if ((*s++ = getach()) == 0 || (*s++ = getach()) == 0)
		return(0);
	*s = '\0';
	if ((j = findch(temp)) > 0)
		return j | chbits;
	else
		return 0;
}

tchar 
setabs (void)		/* set absolute char from \C'...' */
{			/* for now, a no-op */
	int n;

	getch();
	n = 0;
	n = inumb(&n);
	getch();
	if (nonumb)
		return 0;
	return n + nchtab + _SPECCHAR_ST;
}

int 
findft(register int i)
{
	register int k;

	if ((k = i - '0') >= 0 && k <= nfonts && k < smnt)
		return(k);
	for (k = 0; fontlab[k] != i; k++)
		if (k > nfonts)
			return(-1);
	return(k);
}


void
caseps(void)
{
}


void
mchbits(void)
{
	chbits = 0;
	setfbits(chbits, font);
	sps = width(' ' | chbits);
}


void
setps(void)
{
	register int i, j;

	i = cbits(getch());
	if (ischar(i) && isdigit(i)) {		/* \sd or \sdd */
		i -= '0';
		if (i == 0)		/* \s0 */
			;
		else if (i <= 3 && ischar(j = cbits(ch = getch())) &&
		    isdigit(j)) {	/* \sdd */
			ch = 0;
		}
	} else if (i == '(') {		/* \s(dd */
		getch();
		getch();
	} else if (i == '+' || i == '-') {	/* \s+, \s- */
		j = cbits(getch());
		if (ischar(j) && isdigit(j)) {		/* \s+d, \s-d */
			;
		} else if (j == '(') {		/* \s+(dd, \s-(dd */
			getch();
			getch();
		}
	}
}


tchar 
setht (void)		/* set character height from \H'...' */
{
	int	n;

	getch();
	n = inumb(&apts);
	getch();
	return(0);
}


tchar 
setslant (void)		/* set slant from \S'...' */
{
	int	n;

	getch();
	n = 0;
	n = inumb(&n);
	getch();
	return(0);
}


void
caseft(void)
{
	skip();
	setfont(1);
}


void
setfont(int a)
{
	register int i, j;

	if (a)
		i = getrq();
	else 
		i = getsn();
	if (!i || i == 'P') {
		j = font1;
		goto s0;
	}
	if (i == 'S' || i == '0')
		return;
	if ((j = findft(i)) == -1)
		return;
s0:
	font1 = font;
	font = j;
	mchbits();
}


void
setwd(void)
{
	register int base, wid;
	register tchar i;
	int	delim, emsz, k;
	int	savhp, savapts, savapts1, savfont, savfont1, savpts, savpts1;

	base = numtab[ST].val = numtab[ST].val = wid = numtab[CT].val = 0;
	if (ismot(i = getch()))
		return;
	delim = cbits(i);
	savhp = numtab[HP].val;
	numtab[HP].val = 0;
	savapts = apts;
	savapts1 = apts1;
	savfont = font;
	savfont1 = font1;
	savpts = pts;
	savpts1 = pts1;
	setwdf++;
	while (cbits(i = getch()) != delim && !nlflg) {
		k = width(i);
		wid += k;
		numtab[HP].val += k;
		if (!ismot(i)) {
			emsz = (INCH * pts + 36) / 72;
		} else if (isvmot(i)) {
			k = absmot(i);
			if (isnmot(i))
				k = -k;
			base -= k;
			emsz = 0;
		} else 
			continue;
		if (base < numtab[SB].val)
			numtab[SB].val = base;
		if ((k = base + emsz) > numtab[ST].val)
			numtab[ST].val = k;
	}
	setn1(wid, 0, (tchar) 0);
	numtab[HP].val = savhp;
	apts = savapts;
	apts1 = savapts1;
	font = savfont;
	font1 = savfont1;
	pts = savpts;
	pts1 = savpts1;
	mchbits();
	setwdf = 0;
}


tchar 
vmot(void)
{
	dfact = lss;
	vflag++;
	return(mot());
}


tchar 
hmot(void)
{
	dfact = EM;
	return(mot());
}


tchar 
mot(void)
{
	register int j, n;
	register tchar i;

	j = HOR;
	getch(); /*eat delim*/
	if (n = atoi()) {
		if (vflag)
			j = VERT;
		i = makem(quant(n, j));
	} else
		i = 0;
	getch();
	vflag = 0;
	dfact = 1;
	return(i);
}


tchar 
sethl(int k)
{
	register int j;
	tchar i;

	j = t.Halfline;
	if (k == 'u')
		j = -j;
	else if (k == 'r')
		j = -2 * j;
	vflag++;
	i = makem(j);
	vflag = 0;
	return(i);
}


tchar 
makem(int i)
{
	register tchar j;

	if ((j = i) < 0)
		j = -j;
	j |= MOT;
	if (i < 0)
		j |= NMOT;
	if (vflag)
		j |= VMOT;
	return(j);
}


tchar 
getlg(tchar i)
{
	return(i);
}


void
caselg(void)
{
}


void
casefp(void)
{
	register int i, j;

	skip();
	if ((i = cbits(getch()) - '0') < 0 || i > nfonts)
		return;
	if (skip() || !(j = getrq()))
		return;
	fontlab[i] = j;
}


void
casecs(void)
{
}


void
casebd(void)
{
	register int i, j = 0, k;

	k = 0;
bd0:
	if (skip() || !(i = getrq()) || (j = findft(i)) == -1) {
		if (k)
			goto bd1;
		else 
			return;
	}
	if (j == smnt) {
		k = smnt;
		goto bd0;
	}
	if (k) {
		sbold = j;
		j = k;
	}
bd1:
	skip();
	noscale++;
	bdtab[j] = atoi();
	noscale = 0;
}


void
casevs(void)
{
	register int i;

	skip();
	vflag++;
	dfact = INCH; /*default scaling is points!*/
	dfactd = 72;
	res = VERT;
	i = inumb(&lss);
	if (nonumb)
		i = lss1;
	if (i < VERT)
		i = VERT;	/* was VERT */
	lss1 = lss;
	lss = i;
}




void
casess(void)
{
}


tchar 
xlss(void)
{
	/* stores \x'...' into
	 * two successive tchars.
	 * the first contains HX, the second the value,
	 * encoded as a vertical motion.
	 * decoding is done in n2.c by pchar().
	 */
	int	i;

	getch();
	dfact = lss;
	i = quant(atoi(), VERT);
	dfact = 1;
	getch();
	if (i >= 0)
		*pbp++ = MOT | VMOT | i;
	else
		*pbp++ = MOT | VMOT | NMOT | -i;
	return(HX);
}