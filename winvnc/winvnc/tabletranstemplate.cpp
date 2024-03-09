/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


/*
 * tabletranstemplate.c - template for translation using lookup tables.
 *
 * This file shouldn't be compiled. It is included multiple times by
 * translate.c, each time with different definitions of the macros IN and OUT.
 *
 * For each pair of values IN and OUT, this file defines two functions for
 * translating a given rectangle of pixel data. One uses a single lookup
 * table, and the other uses three separate lookup tables for the red, green
 * and blue values.
 *
 * I know this code isn't nice to read because of all the macros, but
 * efficiency is important here.
 */

#if !defined(INVNC) || !defined(OUTVNC)
#error "This file shouldn't be compiled."
#error "It is included as part of translate.c"
#endif

#define IN_T CONCAT2E(CARD,INVNC)
#define OUT_T CONCAT2E(CARD,OUTVNC)
#define rfbTranslateWithSingleTableINVNCtoOUTVNC \
				CONCAT4E(rfbTranslateWithSingleTable,INVNC,to,OUTVNC)
#define rfbTranslateWithRGBTablesINVNCtoOUTVNC \
				CONCAT4E(rfbTranslateWithRGBTables,INVNC,to,OUTVNC)

/*
 * rfbTranslateWithSingleTableINVNCtoOUTVNC translates a rectangle of pixel data
 * using a single lookup table.
 */

static void
rfbTranslateWithSingleTableINVNCtoOUTVNC (char *table, rfbPixelFormat *in,
				    rfbPixelFormat *out,
				    char *iptr, char *optr,
				    int bytesBetweenInputLines,
				    int width, int height)
{
    IN_T *ip = (IN_T *)iptr;
    OUT_T *op = (OUT_T *)optr;
    int ipextra = bytesBetweenInputLines / sizeof(IN_T) - width;
    OUT_T *opLineEnd;
    OUT_T *t = (OUT_T *)table;

    while (height > 0) {
	opLineEnd = op + width;

	while (op < opLineEnd) {
	    *(op++) = t[*(ip++)];
	}

	ip += ipextra;
	height--;
    }
}


/*
 * rfbTranslateWithRGBTablesINVNCtoOUTVNC translates a rectangle of pixel data
 * using three separate lookup tables for the red, green and blue values.
 */

static void
rfbTranslateWithRGBTablesINVNCtoOUTVNC (char *table, rfbPixelFormat *in,
				  rfbPixelFormat *out,
				  char *iptr, char *optr,
				  int bytesBetweenInputLines,
				  int width, int height)
{
    IN_T *ip = (IN_T *)iptr;
    OUT_T *op = (OUT_T *)optr;
    int ipextra = bytesBetweenInputLines / sizeof(IN_T) - width;
    OUT_T *opLineEnd;
    OUT_T *redTable = (OUT_T *)table;
    OUT_T *greenTable = redTable + in->redMax + 1;
    OUT_T *blueTable = greenTable + in->greenMax + 1;

    while (height > 0) {
	opLineEnd = op + width;

	while (op < opLineEnd) {
	    *(op++) = (redTable[(*ip >> in->redShift) & in->redMax] |
		       greenTable[(*ip >> in->greenShift) & in->greenMax] |
		       blueTable[(*ip >> in->blueShift) & in->blueMax]);
	    ip++;
	}
	ip += ipextra;
	height--;
    }
}

#undef IN_T
#undef OUT_T
#undef rfbTranslateWithSingleTableINVNCtoOUTVNC
#undef rfbTranslateWithRGBTablesINVNCtoOUTVNC
