/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*

count leading zeroes function and those that can be derived from it

*/


#ifndef _CLZ_
#define _CLZ_

#include "SC_Types.h"

#define __PPC__ 1
#define __X86__ 0

#ifdef __MWERKS__

// powerpc native count leading zeroes instruction:
#define CLZ(x)  ((int)__cntlzw((unsigned int)x))

#else

static __inline__ int CLZ(int arg) {
#if defined(__ppc__)
         __asm__ volatile("cntlzw %0, %1" : "=r" (arg) : "r" (arg));
#elif defined(__i386__)
         __asm__ volatile("bsrl %0, %0\n\txorl $31,%0\n" : "=r" (arg) : 
"0" (arg)
);
#endif
         return arg;
}

#endif

// count trailing zeroes
inline int32 CTZ(int32 x) 
{
	return 32 - CLZ(~x & (x-1));
}

// count leading ones
inline int32 CLO(int32 x) 
{
	return CLZ(~x);
}

// count trailing ones
inline int32 CTO(int32 x) 
{
	return 32 - CLZ(x & (~x-1));
}

// number of bits required to represent x. 
inline int32 NUMBITS(int32 x) 
{
	return 32 - CLZ(x);
}

// log2 of the next power of two greater than or equal to x. 
inline int32 LOG2CEIL(int32 x) 
{
	return 32 - CLZ(x - 1);
}

// next power of two greater than or equal to x
inline int32 NEXTPOWEROFTWO(int32 x) 
{
	return 1L << LOG2CEIL(x);
}

// is x a power of two
inline bool ISPOWEROFTWO(int32 x) 
{
	return (x & (x-1)) == 0;
}

// input a series of counting integers, outputs a series of gray codes .
inline int32 GRAYCODE(int32 x)
{
	return x & (x>>1);
}

// find least significant bit
inline int32 LSBit(int32 x)
{
	return x & -x;
}

// count number of one bits
inline uint32 ONES(uint32 x) 
{
	uint32 t; 
	x = x - ((x >> 1) & 0x55555555); 
	t = ((x >> 2) & 0x33333333); 
	x = (x & 0x33333333) + t; 
	x = (x + (x >> 4)) & 0x0F0F0F0F; 
	x = x + (x << 8); 
	x = x + (x << 16); 
	return x >> 24; 
}

// count number of zero bits
inline uint32 ZEROES(uint32 x)
{
	return ONES(~x);
}

#endif

