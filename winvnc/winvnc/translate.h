// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


/* translate.h - prototypes of functions in translate.cpp */

#ifndef TRANSLATE_H__
#define TRANSLATE_H__

#include "stdhdrs.h"
#include "rfb.h"
#include "vncmemcpy.h"

// Translate function prototype!
typedef void (*rfbTranslateFnType)(char *table, rfbPixelFormat *in,
				   rfbPixelFormat *out,
				   char *iptr, char *optr,
				   int bytesBetweenInputLines,
				   int width, int height);

// Init function prototype!
typedef void (*rfbInitTableFnType)(char **table, rfbPixelFormat *in,
				   rfbPixelFormat *out);


// External translation stuff
extern void rfbTranslateNone(char *table, rfbPixelFormat *in,
			     rfbPixelFormat *out,
			     char *iptr, char *optr,
			     int bytesBetweenInputLines,
			     int width, int height);

extern HDC GetDcMirror();

// Macro to compare pixel formats.
#define PF_EQ(x,y)												\
	((x.bitsPerPixel == y.bitsPerPixel) &&						\
	 (x.depth == y.depth) &&									\
	 (x.trueColour == y.trueColour) &&							\
	 ((x.bigEndian == y.bigEndian) || (x.bitsPerPixel == 8)) &&	\
	 (!x.trueColour || ((x.redMax == y.redMax) &&				\
			   (x.greenMax == y.greenMax) &&					\
			   (x.blueMax == y.blueMax) &&						\
			   (x.redShift == y.redShift) &&					\
			   (x.greenShift == y.greenShift) &&				\
			   (x.blueShift == y.blueShift))))

// Translation functions themselves
extern rfbInitTableFnType rfbInitTrueColourSingleTableFns[];
extern rfbInitTableFnType rfbInitColourMapSingleTableFns[];
extern rfbInitTableFnType rfbInitTrueColourRGBTablesFns[];
extern rfbTranslateFnType rfbTranslateWithSingleTableFns[3][3];
extern rfbTranslateFnType rfbTranslateWithRGBTablesFns[3][3];
/*
extern Bool rfbSetTranslateFunction(rfbClientPtr cl);
extern void rfbSetClientColourMaps(int firstColour, int nColours);
extern Bool rfbSetClientColourMap(rfbClientPtr cl, int firstColour,
				  int nColours);
*/

#endif