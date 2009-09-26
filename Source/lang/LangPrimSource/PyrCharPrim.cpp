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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
/*

Primitives for Char.

*/

#include <ctype.h>
#include "PyrPrimitive.h"
#include "VMGlobals.h"


int prToLower(struct VMGlobals *g, int numArgsPushed);
int prToLower(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	a->uc = tolower(a->uc);

	return errNone;
}

int prToUpper(struct VMGlobals *g, int numArgsPushed);
int prToUpper(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	a->uc = toupper(a->uc);

	return errNone;
}

int prIsAlpha(struct VMGlobals *g, int numArgsPushed);
int prIsAlpha(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (isalpha(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prIsAlphaNum(struct VMGlobals *g, int numArgsPushed);
int prIsAlphaNum(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (isalnum(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prIsControl(struct VMGlobals *g, int numArgsPushed);
int prIsControl(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (iscntrl(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prIsDigit(struct VMGlobals *g, int numArgsPushed);
int prIsDigit(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (isdigit(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prIsPrint(struct VMGlobals *g, int numArgsPushed);
int prIsPrint(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (isprint(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prIsPunct(struct VMGlobals *g, int numArgsPushed);
int prIsPunct(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (ispunct(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prIsSpace(struct VMGlobals *g, int numArgsPushed);
int prIsSpace(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	if (isspace(a->uc)) { SetTrue(a); }
	else { SetFalse(a); }

	return errNone;
}

int prAsciiValue(struct VMGlobals *g, int numArgsPushed);
int prAsciiValue(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

	a->utag = tagInt;

	return errNone;
}

int prDigitValue(struct VMGlobals *g, int numArgsPushed);
int prDigitValue(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;
	char c;

	a = g->sp;

	c = a->uc;
	if (c >= '0' && c <= '9') {
		a->utag = tagInt;
		a->uc = c - '0';
	} else if (c >= 'a' && c <= 'z') {
		a->utag = tagInt;
		a->uc = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'Z') {
		a->utag = tagInt;
		a->uc = c - 'A' + 10;
	} else {
		return errFailed;
	}

	return errNone;
}


int prAsAscii(struct VMGlobals *g, int numArgsPushed);
int prAsAscii(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;
	a->utag = tagChar;
	a->ui = a->ui & 255;

	return errNone;
}

int prAsDigit(struct VMGlobals *g, int numArgsPushed);
int prAsDigit(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;
	int c;

	a = g->sp;

	c = a->ui;
	if (c >= 0 && c <= 9) {
		a->utag = tagChar;
		a->ui += '0';
	} else if (c >= 10 && c <= 35) {
		a->utag = tagChar;
		a->ui += 'A' - 10;
	} else {
		return errFailed;
	}

	return errNone;
}

void initCharPrimitives();
void initCharPrimitives()
{
	int base, index = 0;

	base = nextPrimitiveIndex();

	definePrimitive(base, index++, "_AsciiValue", prAsciiValue, 1, 0);
	definePrimitive(base, index++, "_DigitValue", prDigitValue, 1, 0);
	definePrimitive(base, index++, "_AsAscii", prAsAscii, 1, 0);
	definePrimitive(base, index++, "_AsDigit", prAsDigit, 1, 0);
	definePrimitive(base, index++, "_ToLower", prToLower, 1, 0);
	definePrimitive(base, index++, "_ToUpper", prToUpper, 1, 0);
	definePrimitive(base, index++, "_IsAlpha", prIsAlpha, 1, 0);
	definePrimitive(base, index++, "_IsAlphaNum", prIsAlphaNum, 1, 0);
	definePrimitive(base, index++, "_IsPrint", prIsPrint, 1, 0);
	definePrimitive(base, index++, "_IsPunct", prIsPunct, 1, 0);
	definePrimitive(base, index++, "_IsControl", prIsControl, 1, 0);
	definePrimitive(base, index++, "_IsSpace", prIsSpace, 1, 0);
	definePrimitive(base, index++, "_IsDecDigit", prIsDigit, 1, 0);

}



#if _SC_PLUGINS_


#include "SCPlugin.h"

// export the function that SC will call to load the plug in.
#pragma export on
extern "C" { SCPlugIn* loadPlugIn(void); }
#pragma export off


// define plug in object
class APlugIn : public SCPlugIn
{
public:
	APlugIn();
	virtual ~APlugIn();

	virtual void AboutToCompile();
};

APlugIn::APlugIn()
{
	// constructor for plug in
}

APlugIn::~APlugIn()
{
	// destructor for plug in
}

void APlugIn::AboutToCompile()
{
	// this is called each time the class library is compiled.
	initCharPrimitives();
}

// This function is called when the plug in is loaded into SC.
// It returns an instance of APlugIn.
SCPlugIn* loadPlugIn()
{
	return new APlugIn();
}

#endif
