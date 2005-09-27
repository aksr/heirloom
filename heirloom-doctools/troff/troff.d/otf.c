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
 * Sccsid @(#)otf.c	1.2 (gritter) 9/28/05
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include <setjmp.h>
#include "dev.h"
#include "afm.h"

extern	struct dev	dev;
extern	char		*chname;
extern	short		*chtab;
extern	int		nchtab;

extern	void	errprint(const char *, ...);
extern	void	verrprint(const char *, va_list);
extern	void	fdprintf(int, char *, ...);
extern	int	stderr;

static jmp_buf	breakpoint;
static char	*contents;
static size_t	size;
static unsigned short	numTables;
static int	ttf;
static const char	*filename;
unsigned short	unitsPerEm;
static struct afmtab	*a;
static int	nc;

static struct table_directory {
	char	tag[4];
	unsigned long	checkSum;
	unsigned long	offset;
	unsigned long	length;
} *table_directory;
static int	pos_CFF;
static int	pos_head;
static int	pos_hmtx;
static int	pos_OS_2;
static int	pos_GSUB;

static unsigned short	*gid2sid;

struct INDEX {
	unsigned short	count;
	int	offSize;
	int	*offset;
	char	*data;
};

static struct CFF {
	struct INDEX	*Name;
	struct INDEX	*Top_DICT;
	struct INDEX	*String;
	struct INDEX	*Global_Subr;
	struct INDEX	*CharStrings;
	int	Charset;
	int	baseoffset;
} CFF;

static const int ExpertCharset[] = {
	0, 1, 229, 230, 231, 232, 233, 234, 235,
	236, 237, 238, 13, 14, 15, 99, 239, 240,
	241, 242, 243, 244, 245, 246, 247, 248, 27,
	28, 249, 250, 251, 252, 253, 254, 255, 256,
	257, 258, 259, 260, 261, 262, 263, 264, 265,
	266, 109, 110, 267, 268, 269, 270, 271, 272,
	273, 274, 275, 276, 277, 278, 279, 280, 281,
	282, 283, 284, 285, 286, 287, 288, 289, 290,
	291, 292, 293, 294, 295, 296, 297, 298, 299,
	300, 301, 302, 303, 304, 305, 306, 307, 308,
	309, 310, 311, 312, 313, 314, 315, 316, 317,
	318, 158, 155, 163, 319, 320, 321, 322, 323,
	324, 325, 326, 150, 164, 169, 327, 328, 329,
	330, 331, 332, 333, 334, 335, 336, 337, 338,
	339, 340, 341, 342, 343, 344, 345, 346, 347,
	348, 349, 350, 351, 352, 353, 354, 355, 356,
	357, 358, 359, 360, 361, 362, 363, 364, 365,
	366, 367, 368, 369, 370, 371, 372, 373, 374,
	375, 376, 377, 378
};

static const int ExpertSubsetCharset[] = {
	0, 1, 231, 232, 235, 236, 237, 238, 13,
	14, 15, 99, 239, 240, 241, 242, 243, 244,
	245, 246, 247, 248, 27, 28, 249, 250, 251,
	253, 254, 255, 256, 257, 258, 259, 260, 261,
	262, 263, 264, 265, 266, 109, 110, 267, 268,
	269, 270, 272, 300, 301, 302, 305, 314, 315,
	158, 155, 163, 320, 321, 322, 323, 324, 325,
	326, 150, 164, 169, 327, 328, 329, 330, 331,
	332, 333, 334, 335, 336, 337, 338, 339, 340,
	341, 342, 343, 344, 345, 346
};

static const char *const StandardStrings[] = {
	".notdef",	/* 0 */
	"space",	/* 1 */
	"exclam",	/* 2 */
	"quotedbl",	/* 3 */
	"numbersign",	/* 4 */
	"dollar",	/* 5 */
	"percent",	/* 6 */
	"ampersand",	/* 7 */
	"quoteright",	/* 8 */
	"parenleft",	/* 9 */
	"parenright",	/* 10 */
	"asterisk",	/* 11 */
	"plus",	/* 12 */
	"comma",	/* 13 */
	"hyphen",	/* 14 */
	"period",	/* 15 */
	"slash",	/* 16 */
	"zero",	/* 17 */
	"one",	/* 18 */
	"two",	/* 19 */
	"three",	/* 20 */
	"four",	/* 21 */
	"five",	/* 22 */
	"six",	/* 23 */
	"seven",	/* 24 */
	"eight",	/* 25 */
	"nine",	/* 26 */
	"colon",	/* 27 */
	"semicolon",	/* 28 */
	"less",	/* 29 */
	"equal",	/* 30 */
	"greater",	/* 31 */
	"question",	/* 32 */
	"at",	/* 33 */
	"A",	/* 34 */
	"B",	/* 35 */
	"C",	/* 36 */
	"D",	/* 37 */
	"E",	/* 38 */
	"F",	/* 39 */
	"G",	/* 40 */
	"H",	/* 41 */
	"I",	/* 42 */
	"J",	/* 43 */
	"K",	/* 44 */
	"L",	/* 45 */
	"M",	/* 46 */
	"N",	/* 47 */
	"O",	/* 48 */
	"P",	/* 49 */
	"Q",	/* 50 */
	"R",	/* 51 */
	"S",	/* 52 */
	"T",	/* 53 */
	"U",	/* 54 */
	"V",	/* 55 */
	"W",	/* 56 */
	"X",	/* 57 */
	"Y",	/* 58 */
	"Z",	/* 59 */
	"bracketleft",	/* 60 */
	"backslash",	/* 61 */
	"bracketright",	/* 62 */
	"asciicircum",	/* 63 */
	"underscore",	/* 64 */
	"quoteleft",	/* 65 */
	"a",	/* 66 */
	"b",	/* 67 */
	"c",	/* 68 */
	"d",	/* 69 */
	"e",	/* 70 */
	"f",	/* 71 */
	"g",	/* 72 */
	"h",	/* 73 */
	"i",	/* 74 */
	"j",	/* 75 */
	"k",	/* 76 */
	"l",	/* 77 */
	"m",	/* 78 */
	"n",	/* 79 */
	"o",	/* 80 */
	"p",	/* 81 */
	"q",	/* 82 */
	"r",	/* 83 */
	"s",	/* 84 */
	"t",	/* 85 */
	"u",	/* 86 */
	"v",	/* 87 */
	"w",	/* 88 */
	"x",	/* 89 */
	"y",	/* 90 */
	"z",	/* 91 */
	"braceleft",	/* 92 */
	"bar",	/* 93 */
	"braceright",	/* 94 */
	"asciitilde",	/* 95 */
	"exclamdown",	/* 96 */
	"cent",	/* 97 */
	"sterling",	/* 98 */
	"fraction",	/* 99 */
	"yen",	/* 100 */
	"florin",	/* 101 */
	"section",	/* 102 */
	"currency",	/* 103 */
	"quotesingle",	/* 104 */
	"quotedblleft",	/* 105 */
	"guillemotleft",	/* 106 */
	"guilsinglleft",	/* 107 */
	"guilsinglright",	/* 108 */
	"fi",	/* 109 */
	"fl",	/* 110 */
	"endash",	/* 111 */
	"dagger",	/* 112 */
	"daggerdbl",	/* 113 */
	"periodcentered",	/* 114 */
	"paragraph",	/* 115 */
	"bullet",	/* 116 */
	"quotesinglbase",	/* 117 */
	"quotedblbase",	/* 118 */
	"quotedblright",	/* 119 */
	"guillemotright",	/* 120 */
	"ellipsis",	/* 121 */
	"perthousand",	/* 122 */
	"questiondown",	/* 123 */
	"grave",	/* 124 */
	"acute",	/* 125 */
	"circumflex",	/* 126 */
	"tilde",	/* 127 */
	"macron",	/* 128 */
	"breve",	/* 129 */
	"dotaccent",	/* 130 */
	"dieresis",	/* 131 */
	"ring",	/* 132 */
	"cedilla",	/* 133 */
	"hungarumlaut",	/* 134 */
	"ogonek",	/* 135 */
	"caron",	/* 136 */
	"emdash",	/* 137 */
	"AE",	/* 138 */
	"ordfeminine",	/* 139 */
	"Lslash",	/* 140 */
	"Oslash",	/* 141 */
	"OE",	/* 142 */
	"ordmasculine",	/* 143 */
	"ae",	/* 144 */
	"dotlessi",	/* 145 */
	"lslash",	/* 146 */
	"oslash",	/* 147 */
	"oe",	/* 148 */
	"germandbls",	/* 149 */
	"onesuperior",	/* 150 */
	"logicalnot",	/* 151 */
	"mu",	/* 152 */
	"trademark",	/* 153 */
	"Eth",	/* 154 */
	"onehalf",	/* 155 */
	"plusminus",	/* 156 */
	"Thorn",	/* 157 */
	"onequarter",	/* 158 */
	"divide",	/* 159 */
	"brokenbar",	/* 160 */
	"degree",	/* 161 */
	"thorn",	/* 162 */
	"threequarters",	/* 163 */
	"twosuperior",	/* 164 */
	"registered",	/* 165 */
	"minus",	/* 166 */
	"eth",	/* 167 */
	"multiply",	/* 168 */
	"threesuperior",	/* 169 */
	"copyright",	/* 170 */
	"Aacute",	/* 171 */
	"Acircumflex",	/* 172 */
	"Adieresis",	/* 173 */
	"Agrave",	/* 174 */
	"Aring",	/* 175 */
	"Atilde",	/* 176 */
	"Ccedilla",	/* 177 */
	"Eacute",	/* 178 */
	"Ecircumflex",	/* 179 */
	"Edieresis",	/* 180 */
	"Egrave",	/* 181 */
	"Iacute",	/* 182 */
	"Icircumflex",	/* 183 */
	"Idieresis",	/* 184 */
	"Igrave",	/* 185 */
	"Ntilde",	/* 186 */
	"Oacute",	/* 187 */
	"Ocircumflex",	/* 188 */
	"Odieresis",	/* 189 */
	"Ograve",	/* 190 */
	"Otilde",	/* 191 */
	"Scaron",	/* 192 */
	"Uacute",	/* 193 */
	"Ucircumflex",	/* 194 */
	"Udieresis",	/* 195 */
	"Ugrave",	/* 196 */
	"Yacute",	/* 197 */
	"Ydieresis",	/* 198 */
	"Zcaron",	/* 199 */
	"aacute",	/* 200 */
	"acircumflex",	/* 201 */
	"adieresis",	/* 202 */
	"agrave",	/* 203 */
	"aring",	/* 204 */
	"atilde",	/* 205 */
	"ccedilla",	/* 206 */
	"eacute",	/* 207 */
	"ecircumflex",	/* 208 */
	"edieresis",	/* 209 */
	"egrave",	/* 210 */
	"iacute",	/* 211 */
	"icircumflex",	/* 212 */
	"idieresis",	/* 213 */
	"igrave",	/* 214 */
	"ntilde",	/* 215 */
	"oacute",	/* 216 */
	"ocircumflex",	/* 217 */
	"odieresis",	/* 218 */
	"ograve",	/* 219 */
	"otilde",	/* 220 */
	"scaron",	/* 221 */
	"uacute",	/* 222 */
	"ucircumflex",	/* 223 */
	"udieresis",	/* 224 */
	"ugrave",	/* 225 */
	"yacute",	/* 226 */
	"ydieresis",	/* 227 */
	"zcaron",	/* 228 */
	"exclamsmall",	/* 229 */
	"Hungarumlautsmall",	/* 230 */
	"dollaroldstyle",	/* 231 */
	"dollarsuperior",	/* 232 */
	"ampersandsmall",	/* 233 */
	"Acutesmall",	/* 234 */
	"parenleftsuperior",	/* 235 */
	"parenrightsuperior",	/* 236 */
	"twodotenleader",	/* 237 */
	"onedotenleader",	/* 238 */
	"zerooldstyle",	/* 239 */
	"oneoldstyle",	/* 240 */
	"twooldstyle",	/* 241 */
	"threeoldstyle",	/* 242 */
	"fouroldstyle",	/* 243 */
	"fiveoldstyle",	/* 244 */
	"sixoldstyle",	/* 245 */
	"sevenoldstyle",	/* 246 */
	"eightoldstyle",	/* 247 */
	"nineoldstyle",	/* 248 */
	"commasuperior",	/* 249 */
	"threequartersemdash",	/* 250 */
	"periodsuperior",	/* 251 */
	"questionsmall",	/* 252 */
	"asuperior",	/* 253 */
	"bsuperior",	/* 254 */
	"centsuperior",	/* 255 */
	"dsuperior",	/* 256 */
	"esuperior",	/* 257 */
	"isuperior",	/* 258 */
	"lsuperior",	/* 259 */
	"msuperior",	/* 260 */
	"nsuperior",	/* 261 */
	"osuperior",	/* 262 */
	"rsuperior",	/* 263 */
	"ssuperior",	/* 264 */
	"tsuperior",	/* 265 */
	"ff",	/* 266 */
	"ffi",	/* 267 */
	"ffl",	/* 268 */
	"parenleftinferior",	/* 269 */
	"parenrightinferior",	/* 270 */
	"Circumflexsmall",	/* 271 */
	"hyphensuperior",	/* 272 */
	"Gravesmall",	/* 273 */
	"Asmall",	/* 274 */
	"Bsmall",	/* 275 */
	"Csmall",	/* 276 */
	"Dsmall",	/* 277 */
	"Esmall",	/* 278 */
	"Fsmall",	/* 279 */
	"Gsmall",	/* 280 */
	"Hsmall",	/* 281 */
	"Ismall",	/* 282 */
	"Jsmall",	/* 283 */
	"Ksmall",	/* 284 */
	"Lsmall",	/* 285 */
	"Msmall",	/* 286 */
	"Nsmall",	/* 287 */
	"Osmall",	/* 288 */
	"Psmall",	/* 289 */
	"Qsmall",	/* 290 */
	"Rsmall",	/* 291 */
	"Ssmall",	/* 292 */
	"Tsmall",	/* 293 */
	"Usmall",	/* 294 */
	"Vsmall",	/* 295 */
	"Wsmall",	/* 296 */
	"Xsmall",	/* 297 */
	"Ysmall",	/* 298 */
	"Zsmall",	/* 299 */
	"colonmonetary",	/* 300 */
	"onefitted",	/* 301 */
	"rupiah",	/* 302 */
	"Tildesmall",	/* 303 */
	"exclamdownsmall",	/* 304 */
	"centoldstyle",	/* 305 */
	"Lslashsmall",	/* 306 */
	"Scaronsmall",	/* 307 */
	"Zcaronsmall",	/* 308 */
	"Dieresissmall",	/* 309 */
	"Brevesmall",	/* 310 */
	"Caronsmall",	/* 311 */
	"Dotaccentsmall",	/* 312 */
	"Macronsmall",	/* 313 */
	"figuredash",	/* 314 */
	"hypheninferior",	/* 315 */
	"Ogoneksmall",	/* 316 */
	"Ringsmall",	/* 317 */
	"Cedillasmall",	/* 318 */
	"questiondownsmall",	/* 319 */
	"oneeighth",	/* 320 */
	"threeeighths",	/* 321 */
	"fiveeighths",	/* 322 */
	"seveneighths",	/* 323 */
	"onethird",	/* 324 */
	"twothirds",	/* 325 */
	"zerosuperior",	/* 326 */
	"foursuperior",	/* 327 */
	"fivesuperior",	/* 328 */
	"sixsuperior",	/* 329 */
	"sevensuperior",	/* 330 */
	"eightsuperior",	/* 331 */
	"ninesuperior",	/* 332 */
	"zeroinferior",	/* 333 */
	"oneinferior",	/* 334 */
	"twoinferior",	/* 335 */
	"threeinferior",	/* 336 */
	"fourinferior",	/* 337 */
	"fiveinferior",	/* 338 */
	"sixinferior",	/* 339 */
	"seveninferior",	/* 340 */
	"eightinferior",	/* 341 */
	"nineinferior",	/* 342 */
	"centinferior",	/* 343 */
	"dollarinferior",	/* 344 */
	"periodinferior",	/* 345 */
	"commainferior",	/* 346 */
	"Agravesmall",	/* 347 */
	"Aacutesmall",	/* 348 */
	"Acircumflexsmall",	/* 349 */
	"Atildesmall",	/* 350 */
	"Adieresissmall",	/* 351 */
	"Aringsmall",	/* 352 */
	"AEsmall",	/* 353 */
	"Ccedillasmall",	/* 354 */
	"Egravesmall",	/* 355 */
	"Eacutesmall",	/* 356 */
	"Ecircumflexsmall",	/* 357 */
	"Edieresissmall",	/* 358 */
	"Igravesmall",	/* 359 */
	"Iacutesmall",	/* 360 */
	"Icircumflexsmall",	/* 361 */
	"Idieresissmall",	/* 362 */
	"Ethsmall",	/* 363 */
	"Ntildesmall",	/* 364 */
	"Ogravesmall",	/* 365 */
	"Oacutesmall",	/* 366 */
	"Ocircumflexsmall",	/* 367 */
	"Otildesmall",	/* 368 */
	"Odieresissmall",	/* 369 */
	"OEsmall",	/* 370 */
	"Oslashsmall",	/* 371 */
	"Ugravesmall",	/* 372 */
	"Uacutesmall",	/* 373 */
	"Ucircumflexsmall",	/* 374 */
	"Udieresissmall",	/* 375 */
	"Yacutesmall",	/* 376 */
	"Thornsmall",	/* 377 */
	"Ydieresissmall",	/* 378 */
	"001.000",	/* 379 */
	"001.001",	/* 380 */
	"001.002",	/* 381 */
	"001.003",	/* 382 */
	"Black",	/* 383 */
	"Bold",	/* 384 */
	"Book",	/* 385 */
	"Light",	/* 386 */
	"Medium",	/* 387 */
	"Regular",	/* 388 */
	"Roman",	/* 389 */
	"Semibold"	/* 390 */
};

static const int	nStdStrings = 391;

static char	**ExtraStrings;
static char	*ExtraStringSpace;

static char *
getSID(int n)
{
	if (n >= 0 && n < nStdStrings)
		return (char *)StandardStrings[n];
	n -= nStdStrings;
	if (CFF.String && n < CFF.String->count)
		return ExtraStrings[n];
	return NULL;
}

static char *
GID2SID(int gid)
{
	if (gid < 0 || gid >= nc)
		return NULL;
	return getSID(gid2sid[gid]);
}

static void
error(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	verrprint(fmt, ap);
	va_end(ap);
	longjmp(breakpoint, 1);
}

static uint16_t
pbe16(const char *cp)
{
	return (uint16_t)(cp[1]&0377) +
		((uint16_t)(cp[0]&0377) << 8);
}

static uint32_t
pbe24(const char *cp)
{
	return (uint32_t)(cp[3]&0377) +
		((uint32_t)(cp[2]&0377) << 8) +
		((uint32_t)(cp[1]&0377) << 16);
}

static uint32_t
pbe32(const char *cp)
{
	return (uint32_t)(cp[3]&0377) +
		((uint32_t)(cp[2]&0377) << 8) +
		((uint32_t)(cp[1]&0377) << 16) +
		((uint32_t)(cp[0]&0377) << 24);
}

static uint32_t
pbeXX(const char *cp, int n)
{
	switch (n) {
	default:
		error("invalid number size %d", n);
	case 1:
		return *cp&0377;
	case 2:
		return pbe16(cp);
	case 3:
		return pbe24(cp);
	case 4:
		return pbe32(cp);
	}
}

static double
cffoper(long *op)
{
	int	b0;
	int	n = 0;
	double	v = 0;

	b0 = contents[*op]&0377;
	if (b0 >= 32 && b0 <= 246) {
		n = 1;
		v = b0 - 139;
	} else if (b0 >= 247 && b0 <= 250) {
		n = 2;
		v = (b0 - 247) * 256 + (contents[*op+1]&0377) + 108;
	} else if (b0 >= 251 && b0 <= 254) {
		n = 2;
		v = -(b0 - 251) * 256 - (contents[*op+1]&0377) - 108;
	} else if (b0 == 28) {
		n = 3;
		v = (int16_t)((contents[*op+1]&0377)<<8 |
				(contents[*op+2]&0377));
	} else if (b0 == 29) {
		n = 5;
		v = (int32_t)pbe32(&contents[*op+1]);
	} else if (b0 == 30) {
		char	buf[100], *xp;
		int	c, i = 0, s = 0;
		n = 1;
		for (;;) {
			if (i == sizeof buf - 2)
				error("floating point operand too long");
			c = (contents[*op+n]&0377) >> (s ? 8 : 0) & 0xf;
			if (c >= 0 && c <= 9)
				buf[i++] = c + '0';
			else if (c == 0xa)
				buf[i++] = '.';
			else if (c == 0xb)
				buf[i++] = 'E';
			else if (c == 0xc) {
				buf[i++] = 'E';
				buf[i++] = '-';
			} else if (c == 0xd)
				error("reserved nibble d in floating point "
						"operand");
			else if (c == 0xe)
				buf[i++] = '-';
			else if (c == 0xf) {
				buf[i++] = 0;
				break;
			}
			if ((s = !s) == 0)
				n++;
		}
		v = strtod(buf, &xp);
		if (*xp != 0)
			error("invalid floating point operand <%s>", buf);
	} else
		error("invalid operand b0 range %d", b0);
	*op += n;
	return v;
}

static void
get_offset_table(void)
{
	char	buf[12];

	if (size < 12)
		error("no offset table");
	memcpy(buf, contents, 12);
	if (pbe32(buf) == 0x00010000) {
		error("cannot handle TrueType-based OpenType fonts");
		ttf = 1;
	} else if (memcmp(buf, "OTTO", 4) == 0) {
		ttf = 0;
	} else
		error("unknown font type");
	numTables = pbe16(&buf[4]);
}

static void
get_table_directory(void)
{
	int	i, o;
	char	buf[16];

	free(table_directory);
	table_directory = calloc(numTables, sizeof *table_directory);
	o = 12;
	pos_CFF = -1;
	pos_head = -1;
	pos_hmtx = -1;
	pos_OS_2 = -1;
	pos_GSUB = -1;
	for (i = 0; i < numTables; i++) {
		if (o + 16 >= size)
			error("cannot get %dth table directory", i);
		memcpy(buf, &contents[o], 16);
		if (memcmp(buf, "CFF ", 4) == 0)
			pos_CFF = i;
		else if (memcmp(buf, "head", 4) == 0)
			pos_head = i;
		else if (memcmp(buf, "hmtx", 4) == 0)
			pos_hmtx = i;
		else if (memcmp(buf, "OS/2", 4) == 0)
			pos_OS_2 = i;
		else if (memcmp(buf, "GSUB", 4) == 0)
			pos_GSUB = i;
		o += 16;
		memcpy(table_directory[i].tag, buf, 4);
		table_directory[i].checkSum = pbe32(&buf[4]);
		table_directory[i].offset = pbe32(&buf[8]);
		table_directory[i].length = pbe32(&buf[12]);
	}
}

static void
free_INDEX(struct INDEX *ip)
{
	if (ip) {
		free(ip->offset);
		free(ip);
	}
}

static struct INDEX *
get_INDEX(long *op)
{
	struct INDEX	*ip;
	int	i;

	if (*op + 3 >= size)
		error("no index at position %ld", *op);
	ip = calloc(1, sizeof *ip);
	ip->count = pbe16(&contents[*op]);
	*op += 2;
	if (ip->count != 0) {
		ip->offSize = contents[(*op)++] & 0377;
		ip->offset = calloc(ip->count+1, sizeof *ip->offset);
		for (i = 0; i < ip->count+1; i++) {
			if (*op + ip->offSize >= size) {
				free_INDEX(ip);
				error("no index offset at position %ld", *op);
			}
			ip->offset[i] = pbeXX(&contents[*op], ip->offSize);
			*op += ip->offSize;
		}
		ip->data = &contents[*op];
		for (i = 0; i < ip->count+1; i++)
			ip->offset[i] += *op - 1;
		*op = ip->offset[ip->count];
	}
	return ip;
}

static void
onechar(int gid, int sid)
{
	long	o;
	int	w, tp;
	char	*N;
	int	B[4] = { 0, 0, 0, 0};

	if (pos_hmtx < 0)
		error("no hmtx table");
	o = table_directory[pos_hmtx].offset;
	if (table_directory[pos_hmtx].length < 4 * (gid+1))
		return;	/* just ignore this glyph */
	w = pbe16(&contents[o + 4 * gid]);
	if ((N = getSID(sid)) != NULL)
		a->nspace += strlen(N) + 1;
	tp = afmmapname(N, 0, 0);
	afmaddchar(a, gid, tp, 0, w, B, N, 0, 0);
	gid2sid[gid] = sid;
}

static int
get_CFF_Top_DICT_Entry(int e)
{
	long	o;
	int	d = 0;

	if (CFF.Top_DICT == NULL || CFF.Top_DICT->offset == NULL)
		error("no Top DICT INDEX");
	o = CFF.Top_DICT->offset[0];
	while (o < CFF.Top_DICT->offset[1] && contents[o] != e) {
		if (contents[o] < 0 || contents[o] > 27)
			d = cffoper(&o);
		else {
			d = 0;
			o++;
		}
	}
	return d;
}

static void
get_CFF_Charset(void)
{
	int	d = 0;
	int	gid, i, j, first, nLeft;

	d = get_CFF_Top_DICT_Entry(15);
	if (d == 0) {
		for (i = 0; i < nc && i <= 228; i++)
			onechar(i, i);
	} else if (d == 1) {
		for (i = 0; i < nc && i <= 166; i++)
			onechar(i, ExpertCharset[i]);
	} else if (d == 2) {
		for (i = 0; i < nc && i <= 87; i++)
			onechar(i, ExpertSubsetCharset[i]);
	} else if (d > 2) {
		d = CFF.Charset = d + CFF.baseoffset;
		onechar(0, 0);
		gid = 1;
		switch (contents[d++]) {
		case 0:
			for (i = 1; i < nc; i++) {
				j = pbe16(&contents[d]);
				d += 2;
				onechar(gid++, j);
			}
			break;
		case 1:
			i = nc - 1;
			while (i > 0) {
				first = pbe16(&contents[d]);
				d += 2;
				nLeft = contents[d++] & 0377;
				for (j = 0; j <= nLeft && gid < nc; j++)
					onechar(gid++, first + j);
				i -= nLeft + 1;
			}
			break;
		default:
			error("unknown Charset table format %d", contents[d-1]);
		case 2:
			i = nc - 1;
			while (i > 0) {
				first = pbe16(&contents[d]);
				d += 2;
				nLeft = pbe16(&contents[d]);
				d += 2;
				for (j = 0; j <= nLeft && gid < nc; j++)
					onechar(gid++, first + j);
				i -= nLeft + 1;
			}
		}
	} else
		error("invalid Charset offset");
}

static void
build_ExtraStrings(void)
{
	int	c, i;
	char	*sp;

	if (CFF.String == NULL || CFF.String->count == 0)
		return;
	ExtraStrings = calloc(CFF.String->count, sizeof *ExtraStrings);
	sp = ExtraStringSpace = malloc(CFF.String->count +
			CFF.String->offset[CFF.String->count]);
	for (c = 0; c < CFF.String->count; c++) {
		ExtraStrings[c] = sp;
		for (i = CFF.String->offset[c];
				i < CFF.String->offset[c+1]; i++)
			*sp++ = contents[i];
		*sp++ = 0;
	}
}

static void
get_CFF(void)
{
	long	o;
	char	buf[4];

	if (pos_CFF < 0)
		error("no CFF table");
	CFF.baseoffset = o = table_directory[pos_CFF].offset;
	if (o + 4 >= size)
		error("no CFF header");
	memcpy(buf, &contents[o], 4);
	o += 4;
	if (buf[0] != 1)
		error("can only handle CFF major version 1");
	CFF.Name = get_INDEX(&o);
	CFF.Top_DICT = get_INDEX(&o);
	CFF.String = get_INDEX(&o);
	build_ExtraStrings();
	CFF.Global_Subr = get_INDEX(&o);
	o = get_CFF_Top_DICT_Entry(17);
	o += CFF.baseoffset;
	CFF.CharStrings = get_INDEX(&o);
	if (CFF.Name->count != 1)
		error("cannot handle CFF data with more than one font");
	a->fontname = malloc(CFF.Name->offset[1] - CFF.Name->offset[0] + 1);
	memcpy(a->fontname, &contents[CFF.Name->offset[0]],
			CFF.Name->offset[1] - CFF.Name->offset[0]);
	a->fontname[CFF.Name->offset[1] - CFF.Name->offset[0]] = 0;
	if (CFF.CharStrings == NULL || CFF.CharStrings->count == 0)
		error("no characters in font");
	nc = CFF.CharStrings->count;
	gid2sid = calloc(nc, sizeof *gid2sid);
	afmalloc(a, nc);
	get_CFF_Charset();
	afmremap(a);
}

static void
get_head(void)
{
	long	o;

	if (pos_head < 0)
		error("no head table");
	o = table_directory[pos_head].offset;
	if (pbe32(&contents[o]) != 0x00010000)
		error("can only handle version 1.0 head tables");
	unitsPerEm = pbe16(&contents[o + 18]);
}

static void
get_OS_2(void)
{
	long	o;

	if (pos_OS_2 < 0)
		goto dfl;
	o = table_directory[pos_OS_2].offset;
	if (pbe16(&contents[o]) > 0x0003)
		goto dfl;
	if (table_directory[pos_OS_2].length >= 98) {
		a->xheight = pbe16(&contents[o + 94]);
		a->capheight = pbe16(&contents[o + 96]);
	} else {
	dfl:	a->xheight = 500;
		a->capheight = 700;
	}
}

static int	ScriptList;
static int	FeatureList;
static int	LookupList;

static void
get_Ligature(int first, int o)
{
	int	LigGlyph;
	int	CompCount;
	int	Component[16];
	int	i;
	char	*gn;

	LigGlyph = pbe16(&contents[o]);
	CompCount = pbe16(&contents[o+2]);
	for (i = 0; i < CompCount - 1 &&
			i < sizeof Component / sizeof *Component - 1; i++) {
		Component[i] = pbe16(&contents[o+4+2*i]);
	}
	Component[i] = -1;
	gn = GID2SID(first);
	if (gn[0] == 'f' && gn[1] == 0 && CompCount > 1) {
		gn = GID2SID(Component[0]);
		if (gn[0] && gn[1] == 0) switch (gn[0]) {
		case 'f':
			if (CompCount == 2) {
				gn = GID2SID(LigGlyph);
				if (strcmp(gn, "ff") == 0)
					a->Font.ligfont |= LFF;
			} else if (CompCount == 3) {
				gn = GID2SID(Component[1]);
				if (gn[0] && gn[1] == 0) switch (gn[0]) {
				case 'i':
					gn = GID2SID(LigGlyph);
					if (strcmp(gn, "ffi") == 0)
						a->Font.ligfont |= LFFI;
					break;
				case 'f':
					gn = GID2SID(LigGlyph);
					if (strcmp(gn, "ffl") == 0)
						a->Font.ligfont |= LFFL;
					break;
				}
			}
			break;
		case 'i':
			if (CompCount == 2) {
				gn = GID2SID(LigGlyph);
				if (strcmp(gn, "fi") == 0)
					a->Font.ligfont |= LFI;
			}
			break;
		case 'l':
			if (CompCount == 2) {
				gn = GID2SID(LigGlyph);
				if (strcmp(gn, "fl") == 0)
					a->Font.ligfont |= LFL;
			}
			break;
		}
	}
}

static void
get_LigatureSet(int first, int o)
{
	int	LigatureCount;
	int	i;

	LigatureCount = pbe16(&contents[o]);
	for (i = 0; i < LigatureCount; i++)
		get_Ligature(first, o + pbe16(&contents[o+2+2*i]));
}

static void
get_LigatureSubstFormat1(int o)
{
	int	Coverage;
	int	GlyphCount;
	int	LigSetCount;
	int	i;
	int	first;

	if (pbe16(&contents[o]) != 1)
		return;
	Coverage = o + pbe16(&contents[o+2]);
	if (pbe16(&contents[Coverage]) != 1)
		return;
	GlyphCount = pbe16(&contents[Coverage+2]);
	LigSetCount = pbe16(&contents[o+4]);
	for (i = 0; i < LigSetCount && i < GlyphCount; i++) {
		first = pbe16(&contents[Coverage+4+2*i]);
		get_LigatureSet(first, o + pbe16(&contents[o+6+2*i]));
	}
}

static void
get_liga(int o)
{
	int	i, j, x, y;
	int	LookupCount;
	int	SubTableCount;

	LookupCount = pbe16(&contents[o+2]);
	for (i = 0; i < LookupCount; i++) {
		x = pbe16(&contents[o+4+2*i]);
		y = pbe16(&contents[LookupList+2+2*x]);
		if (pbe16(&contents[LookupList+y]) == 4) {
			SubTableCount = pbe16(&contents[LookupList+y+4]);
			for (j = 0; j < SubTableCount; j++)
				get_LigatureSubstFormat1(LookupList+y +
					pbe16(&contents[LookupList+y+6+2*j]));
		}
	}
}

static void
get_LangSys(int o)
{
	int	i, x;
	int	FeatureCount;
	int	ReqFeatureIndex;

	ReqFeatureIndex = pbe16(&contents[o+2]);
	FeatureCount = pbe16(&contents[o+4]);
	if (ReqFeatureIndex != 0xFFFF)
		FeatureCount += ReqFeatureIndex;
	for (i = 0; i < FeatureCount; i++) {
		x = pbe16(&contents[o+6+2*i]);
		if (memcmp(&contents[FeatureList+2+6*x], "liga", 4) == 0)
			get_liga(FeatureList +
				pbe16(&contents[FeatureList+2+6*x+4]));
	}
}

static void
get_GSUB(void)
{
	long	o;
	int	i;
	int	DefaultLangSys;
	int	ScriptCount;
	int	Script;

	if (pos_GSUB < 0)
		return;
	o = table_directory[pos_GSUB].offset;
	if (pbe32(&contents[o]) != 0x00010000)
		return;
	ScriptList = o + pbe16(&contents[o+4]);
	FeatureList = o + pbe16(&contents[o+6]);
	LookupList = o + pbe16(&contents[o+8]);
	ScriptCount = pbe16(&contents[ScriptList]);
	for (i = 0; i < ScriptCount; i++)
		if (memcmp(&contents[ScriptList+2+6*i], "DFLT", 4) == 0 ||
				memcmp(&contents[ScriptList+2+6*i],
					"latn", 4) == 0) {
			Script = ScriptList +
				pbe16(&contents[ScriptList+2+6*i+4]);
			DefaultLangSys = Script + pbe16(&contents[Script]);
			get_LangSys(DefaultLangSys);
		}
}

int
otfcff(const char *path,
		char *_contents, size_t _size, size_t *offset, size_t *length)
{
	int	ok = 0;

	a = NULL;
	filename = path;
	contents = _contents;
	size = _size;
	if (setjmp(breakpoint) == 0) {
		get_offset_table();
		get_table_directory();
		if (pos_CFF < 0)
			error("no CFF table");
		*offset = table_directory[pos_CFF].offset;
		*length = table_directory[pos_CFF].length;
	} else
		ok = -1;
	return ok;
}

int
otfget(struct afmtab *_a, char *_contents, size_t _size)
{
	int	ok = 0;

	a = _a;
	filename = a->path;
	contents = _contents;
	size = _size;
	if (setjmp(breakpoint) == 0) {
		get_offset_table();
		get_table_directory();
		get_head();
		get_OS_2();
		if (ttf == 0) {
			get_CFF();
		}
		get_GSUB();
		a->Font.nwfont = a->nchars > 255 ? 255 : a->nchars;
	} else
		ok = -1;
	free_INDEX(CFF.Name);
	CFF.Name = 0;
	free_INDEX(CFF.Top_DICT);
	CFF.Top_DICT = 0;
	free_INDEX(CFF.String);
	CFF.String = 0;
	free_INDEX(CFF.Global_Subr);
	CFF.Global_Subr = 0;
	free_INDEX(CFF.CharStrings);
	CFF.CharStrings = 0;
	free(ExtraStringSpace);
	free(ExtraStrings);
	return ok;
}