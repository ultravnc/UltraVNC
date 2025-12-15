// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


/* mnemonic.c

 Copyright (c) 2000  Oren Tirosh <oren@hishome.net>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*/

#include <ctype.h>
#include <string.h>

#include "mnemonic.h"

/*
 * mn_words_required
 * 
 * Description:
 *  Calculate the number of words required to encode data using mnemonic
 *  encoding.
 *
 * Parameters:
 *  size - Size in bytes of data to be encoded
 * 
 * Return value:
 *  number of words required for the encoding
 */

int
mn_words_required (int size)
{
  return ((size + 1) * 3) / 4;
}


/*
 * mn_encode_word_index
 *
 * Description:
 *  Perform one step of encoding binary data into words. Returns word index.
 *
 * Parameters:
 *   src - Pointer to data buffer to encode
 *   srcsize - Size in bytes of data to encode 
 *   n - Sequence number of word to encode
 *       0 <= n < mn_words_required(srcsize)
 *
 * Return value:
 *   0 - no more words to encode / n is out of range
 *   1..MN_WORDS - word index. May be used as index to the mn_words[] array
 */

mn_index mn_encode_word_index (void *vsrc, int srcsize, int n)
{
  mn_word32 x = 0;		/* Temporary for MN_BASE arithmetic */
  int offset;			/* Offset into src */
  int remaining;		/* Octets remaining to end of src */
  int extra = 0;		/* Index 7 extra words for 24 bit data */
  int i;
  mn_byte *src = vsrc;

  if (n < 0 || n >= mn_words_required (srcsize))
    return 0;			/* word out of range */
  offset = (n / 3) * 4;		/* byte offset into src */
  remaining = srcsize - offset;
  if (remaining <= 0)
    return 0;
  if (remaining >= 4)
    remaining = 4;
  for (i = 0; i < remaining; i++)
    x |= (unsigned long)src[offset + i] << (i * 8);	/* endianness-agnostic */

  switch (n % 3)
    {
    case 2:			/* Third word of group */
      if (remaining == 3)	/*  special case for 24 bits */
	extra = MN_BASE;	/*  use one of the 7 3-letter words */
      x /= (MN_BASE * MN_BASE);
      break;
    case 1:			/* Second word of group */
      x /= MN_BASE;
    }
  return x % MN_BASE + extra + 1;
}


/*
 * mn_encode_word
 *
 * Description:
 *  Perform one step of encoding binary data into words. Returns pointer 
 *  to word.
 *
 * Parameters:
 *   src - Pointer to data buffer to encode
 *   srcsize - Size of data to encode in bytes
 *   n - Sequence number of word to encode. 
 *       0 <= n < mn_words_required(srcsize)
 *
 * Return value:
 *   NULL - no more words to encode / n is out of range
 *   valid pointer - pointer to null-terminated lowercase word. length<=7
 */

const char *
mn_encode_word (void *src, int srcsize, int n)
{
  return mn_words[mn_encode_word_index (src, srcsize, n)];
}


/*
 * mn_next_word_index
 *
 * Description:
 *  Perform one step of decoding a null-terminated buffer into word indices.
 *  A word is defined as a sequence of letter character separated by one
 *  or more non-letter separator characters.
 *
 * Parameters:
 *  ptr - Pointer to a pointer to the next character in the buffer.
 *  *ptr is modified by the function; see Return Value below.
 *
 * Return value:
 *  0  - If *ptr==0 (points to the null at the end of the buffer) no more 
 *       words were found in the buffer. Otherwise *ptr points to beginning 
 *       of an unrecognized word.
 *  >0 - index of word found, suitable for decoding with mn_decode_word_index
 *       or comparison to values returned from mn_encode_index. *ptr points 
 *       to first character of next word or to terminating null.
 */

mn_index
mn_next_word_index (char **ptr)
{
  char *wordstart;
  char wordbuf[MN_WORD_BUFLEN];
  int i = 0;
  char c;
  mn_index idx;

  while (**ptr && !isalpha (**ptr))	/* skip separator chars */
    (*ptr)++;
  wordstart = *ptr;		/* save for error reporting */
  while (**ptr && isalpha (**ptr) && i < MN_WORD_BUFLEN - 1)
    {
      c = *(*ptr)++;
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';		/* convert to lowercase */
      wordbuf[i++] = c;
    }
  wordbuf[i] = '\0';
  while (**ptr && isalpha (**ptr))	/* skip tail of long words */
    (*ptr)++;
  while (**ptr && !isalpha (**ptr))	/* skip separators */
    (*ptr)++;

  if (wordbuf[0] == '\0')
    return 0;			/* EOF, no word found */

  for (idx = 1; idx <= MN_WORDS; idx++)
    {
      if (!strcmp (wordbuf, mn_words[idx]))
	return idx;
      /* FIXME: some fancy code should go here
         to accept misspellings and soundalikes.
         (replacing the linear search would also be nice) */
    }
  *ptr = wordstart;
  return 0;			/* not found */
}


/*
 * mn_decode_word_index
 *
 * Description:
 *  Perform one step of decoding a sequence of words into binary data.
 *
 * Parameters:
 *  index    - Index of word, e.g. return value of mn_next_word_index. Use
 *             the value MN_EOF(=0) to signal the end of input.
 *  dest     - Points to buffer to receive decoded binary result.
 *  destsize - Size of buffer 
 *  offset   - Pointer to an integer offset into the destination buffer for 
 *             next data byte. Initialize *offset to 0 before first call to 
 *             function. Modified by function and may be used as an 
 * 	       indication for the amount of data actually decoded.
 *
 * Return value:
 *  The return value indicates the status of the decoding function. It is
 *  ok to ignore this value on all calls to the function except the last
 *  one (with index=MN_EOF). Any errors encountered will be reported on. 
 *  the last call. The error code is also returned in *offset (negative 
 *  values indicate error).
 *
 * MN_OK (==0)	
 *	for index!=MN_EOF a return value of MN_OK means that 
 *	decoding has been successful so far.
 *	for index==MN_EOF a return value of MN_OK means that decoding
 *	of the entire buffer has been successful and the decoder is in
 *	a valid state for the end of the message. A total of *offset
 *	valid decoded bytes is in the buffer.
 *  MN_EREM      
 *	returned on MN_EOF when an unaccounted arithmetic remainder is
 *	in the decoder. Most likely indicates a truncated word sequence.
 *  MN_EOVERRUN	
 *	Not enough room in buffer for decoded data.
 *  MN_EOVERRUN24 
 *	Returned when decoding of data is attempted after decoding one
 *	of the 7 words reserved for 24 bit remainders at the end of the
 *	message. Probably indicates a garbled messages.
 *  MN_EINDEX	
 *	Bad input index. Naturally this should not happen when using 
 *	the result of mn_next_word_index.
 *  MN_EINDEX24
 *	Returned when one of the 7 words reserved for 24 bit remainders
 *	is received at an offset inappropriate for a 24 bit remainder.
 *  MN_EENCODING
 *	Indicates an overflow in MN_BASE arithmetic. Approximately 0.09%
 *	of the 3 word combinations are unused and will generate this error.
 */

int
mn_decode_word_index (mn_index index, void *vdest, int destsize, int *offset)
{
  mn_word32 x;			/* Temporary for MN_BASE arithmetic */
  int groupofs;
  int i;
  mn_byte *dest = vdest;

  if (*offset < 0)		/* Error from previous call? report it */
    return *offset;

  if (index > MN_WORDS)	/* Word index out of range */
    {
      *offset = MN_EINDEX;
      return *offset;
    }

  if (*offset > destsize)	/* out of range? */
    {
      *offset = MN_EOVERRUN;
      return *offset;
    }

  if (index > MN_BASE && *offset % 4 != 2)
    {				/* Unexpected 24 bit remainder word */
      *offset = MN_EINDEX24;
      return *offset;
    }

  groupofs = *offset & ~3;	/* Offset of 4 byte group containing offet */
  x = 0;
  for (i = 0; i < 4; i++)
    if (groupofs + i < destsize)	/* Ignore any bytes outside buffer */
      x |= (unsigned long)dest[groupofs + i] << (i * 8);	/* assemble number */

  if (index == MN_EOF)		/* Got EOF signal */
    {
      switch (*offset % 4)
	{
	case 3:		/* group was three words and the last */
	  return MN_OK;		/*  word was a 24 bit remainder */
	case 2:		/* last group has two words */
	  if (x <= 0xFFFF)	/*  should encode 16 bit data */
	    return MN_OK;
	  else
	    {
	      *offset = MN_EREM;
	      return *offset;
	    }
	case 1:		/* last group has just one word */
	  if (x <= 0xFF)	/*  should encode 8 bits */
	    return MN_OK;
	  else
	    {
	      *offset = MN_EREM;
	      return *offset;
	    }

	case 0:		/* last group was full 3 words */
	  return MN_OK;
	}
    }
  if (*offset == destsize)	/* At EOF but didn't get MN_EOF */
    {
      *offset = MN_EOVERRUN;
      return *offset;
    }

  index--;			/* 1 based to 0 based index */

  switch (*offset % 4)
    {
    case 3:			/* Got data past 24 bit remainder */
      *offset = MN_EOVERRUN24;
      return *offset;
    case 2:
      if (index >= MN_BASE)
	{			/* 24 bit remainder */
	  x += (index - MN_BASE) * MN_BASE * MN_BASE;
	  (*offset)++;		/* *offset%4 == 3 for next time */
	}
      else
	{			/* catch invalid encodings */
	  if (index >= 1625 || (index == 1624 && x > 1312671))
	    {
	      *offset = MN_EENCODING;
	      return *offset;
	    }
	  x += index * MN_BASE * MN_BASE;
	  (*offset) += 2;	/* *offset%4 == 0 for next time */
	}
      break;
    case 1:
      x += index * MN_BASE;
      (*offset)++;
      break;
    case 0:
      x = index;
      (*offset)++;
      break;
    }

  for (i = 0; i < 4; i++)
    if (groupofs + i < destsize)	/* Don't step outside the buffer */
      {
	dest[groupofs + i] = (mn_byte) x % 256;
	x /= 256;
      }
  return MN_OK;
}

/*
 * mn_encode
 *
 * Description:
 *  Encode a binary data buffer into a null-terminated sequence of words.
 *  The word separators are taken from the format string. 
 *
 * Parameters:
 *  src      - Pointer to the beginning of the binary data buffer.
 *  srcsize  - Size in bytes of binary data buffer
 *  dest     - Pointer to the beginning of a character buffer 
 *  destsize - Size in characters of character buffer
 *  format   - Null-terminated string describing the output format.
 *             In the format string any letter or sequence of letters
 *             acts as a placeholder for the encoded words. The word 
 *             placeholders are separated by one or more non-letter
 *             characters. When the encoder reaches the end of the 
 *             format string it starts reading it again.
 *             For sample formats see MN_F* constants in mnemonic.h
 *	       If format is empty or NULL the format MN_FDEFAULT
 *	       is used.
 *
 * Return value:
 *  MN_OK(=0)
 *	Encoding was successful.
 *  MN_EOVERRUN
 *	Output size exceeds size of destination buffer
 *  MN_EFORMAT
 *	Invalid format string. This function enforces formats which
 *	will result in a string which can be successfully decoded by
 *	the mn_decode function.
 */

int
mn_encode (void *src, int srcsize, char *dest, int destsize, char *format)
{
  int n;
  char *fmt;
  char *destend = dest + destsize;
  const char *word;
  char capitalword[16];
  char *capitalwordPtr;
  memset(capitalword, 0, 16);


  if (format == 0 || format[0] == '\0')
    format = MN_FDEFAULT;
  fmt = format;
  for (n = 0; n < mn_words_required (srcsize); n++)
    {
      while (dest < destend && *fmt != '\0' && !isalpha (*fmt))
	*dest++ = *fmt++;
      if (dest >= destend)
	return MN_EOVERRUN;
      if (*fmt == '\0')
	{
	  if (isalpha (fmt[-1]) && isalpha (format[0]))
	    return MN_EFORMAT;
	  fmt = format;
	  while (dest < destend && *fmt != '\0' && !isalpha (*fmt))
	    *dest++ = *fmt++;
	  if (!isalpha (*fmt))
	    return MN_EFORMAT;
	}
      word = mn_encode_word (src, srcsize, n);
      for (size_t i = 0; i < strlen(word); i++)
        capitalword[i] = (i == 0 && (n == 0 || n == 3)) ? toupper(word[i]) : word[i];
      if (word == 0)
	return MN_EOVERRUN;	/* shouldn't happen, actually */
      capitalwordPtr = capitalword;
      while (isalpha (*fmt))
	fmt++;
      while (dest < destend && *capitalwordPtr != '\0')
	*dest++ = *capitalwordPtr++;
    }
  if (dest < destend)
    *dest++ = '\0';
  else
    return MN_EOVERRUN;
  return MN_OK;
}

/*
 * mn_decode
 *
 * Description:
 *  Decode a text representation in null-terminated character buffer src to 
 *  binary buffer dest.
 *
 * Parameters:
 *  src      - Pointer to null-terminated character buffer 
 *  dest     - Pointer to beginning of destination buffer
 *  destsize - Size in bytes of destination buffer
 *
 * Return value:
 *  This function may return all the value returned by mn_decode_word_index
 *  plus the following result code:
 *
 * MN_EWORD  - Unrecognized word.
 */

int
mn_decode (char *src, void *dest, int destsize)
{
  mn_index index;
  int offset = 0;
  int status;

  while ((index = mn_next_word_index (&src)) != 0)
    {
      (void) mn_decode_word_index (index, dest, destsize, &offset);
    }
  if (*src != 0)
    return MN_EWORD;
  status = mn_decode_word_index (MN_EOF, dest, destsize, &offset);
  if (status < 0)
    return status;
  return offset;
}
