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


#include "SC_Unit.h"
#include "SC_UnitSpec.h"
#include "SC_UnitDef.h"
#include "SC_World.h"
#include "SC_Wire.h"
#include "Unroll.h"
#include <stdio.h>
#include "SC_Prototypes.h"

void Unit_ChooseMulAddFunc(Unit* unit);

Unit* Unit_New(World *inWorld, UnitSpec *inUnitSpec, char*& memory)
{
	UnitDef *def = inUnitSpec->mUnitDef;
	
	Unit *unit = (Unit*)memory; 
	memory += def->mAllocSize;
	
	unit->mWorld = inWorld;
	unit->mUnitDef = def;
	unit->mParent = 0;
	unit->mParentIndex = 0;
	unit->mNumInputs = inUnitSpec->mNumInputs;
	unit->mNumOutputs = inUnitSpec->mNumOutputs;
	int numPorts = unit->mNumInputs + unit->mNumOutputs;

	unit->mInput = (Wire**)memory; 
	memory += numPorts * sizeof(Wire*);
	
	unit->mOutput = unit->mInput + unit->mNumInputs;

	unit->mInBuf = (float**)memory; 
	memory += numPorts * sizeof(float*);

	unit->mOutBuf = unit->mInBuf + unit->mNumInputs;
	

	unit->mCalcRate = inUnitSpec->mCalcRate;
	unit->mSpecialIndex = inUnitSpec->mSpecialIndex;
	
	unit->mRate = 0;
	unit->mDimension = 0;
	unit->mCalcFunc = 0;
	unit->mDone = false;
	
	return unit;
}

void Unit_Dtor(Unit *inUnit)
{
}

void Unit_ZeroOutputs(Unit *unit, int inNumSamples)
{
	long numOuts = unit->mNumOutputs;
	for (int i=0; i<numOuts; ++i) {
		float *out = OUT(i);
		Clear(inNumSamples, out);
	}

}

void Unit_EndCalc(Unit *inUnit, int inNumSamples)
{
	inUnit->mDone = true;
	Unit_ZeroOutputs(inUnit, inNumSamples);
}

void Unit_End(Unit *inUnit)
{
	inUnit->mCalcFunc = (UnitCalcFunc)&Unit_EndCalc;
}

