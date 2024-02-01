/* mnemonic.h 

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

#define MN_BASE		1626		/* cubic root of 2^32, rounded up */
#define MN_REMAINDER	7		/* extra words for 24 bit remainders */
#define MN_WORDS (MN_BASE+MN_REMAINDER)	/* total number of words */
#define MN_WORD_BUFLEN	25		/* size for a word buffer+headroom */

#define MN_EOF		0		/* signal end to mn_decode_word_index */

/* result codes */
#define MN_OK		0		/* decoding ok */
#define MN_EREM		-1		/* unexpected arithmetic remainder */
#define MN_EOVERRUN	-2		/* output buffer overrun */
#define MN_EOVERRUN24	-3		/* data after 24 bit remainder */
#define MN_EINDEX	-4		/* bad word index */
#define MN_EINDEX24	-5		/* unexpected 24 bit remainder word */
#define MN_EENCODING	-6		/* invalid arithmetic encoding */
#define MN_EWORD	-7		/* unrecognized word */
#define MN_EFORMAT	-8		/* bad format string */

/* Sample formats for mn_encode */
#define MN_FDEFAULT 		"x-x-x--"
#define MN_F64BITSPERLINE	" x-x-x--x-x-x\n"
#define MN_F96BITSPERLINE	" x-x-x--x-x-x--x-x-x\n"
#define MN_F128BITSPERLINE	" x-x-x--x-x-x--x-x-x--x-x-x\n"
/* Note that the last format does not fit in a standard 80 character line */

typedef unsigned char mn_byte;		/* 8 bit quantity */
typedef unsigned long mn_word32;	/* temporary value, at least 32 bits */
/* Range checks assume that mn_index is unsigned (=> can't be <0).  --DV */
typedef unsigned int mn_index;		/* index into wordlist */

extern const char *mn_words[];		/* the word list itself */
extern const char *mn_wordlist_version;	/* version notice string */

int 		mn_decode (char *src, void *dest, int destsize);
int 		mn_encode (void *src , int srcsize, 
			   char *dest, int destsize, char *format);

int 		mn_words_required (int size);
mn_index 	mn_encode_word_index (void *src, int srcsize, int n);
const char*	mn_encode_word (void *src, int srcsize, int n);
mn_index 	mn_next_word_index (char **ptr);
int 		mn_decode_word_index (mn_index index, void *dest, 
				     int destsize, int *offset);


