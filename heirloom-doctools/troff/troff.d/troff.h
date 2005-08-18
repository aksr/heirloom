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
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)troff.h	1.1 (gritter) 8/18/05
 */

extern struct tkftab {
	int	s1;
	int	n1;
	int	s2;
	int	n2;
} *tkftab;

extern	int		Nfont;
extern	struct Font	**fontbase;
extern	char		**codetab;
extern	int		*cstab;
extern	int		*ccstab;

extern	int		nchtab;
extern	char		*chname;
extern	short		*chtab;

extern	long		realpage;

extern	void		growfonts(int);