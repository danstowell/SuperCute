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


#include "SC_PlugIn.h"
#include <limits.h>

static InterfaceTable *ft;

struct Vibrato : public Unit
{
	double mPhase, m_attackSlope, m_attackLevel;
	float mFreqMul, m_scaleA, m_scaleB, mFreq;
	int m_delay, m_attack;
};

struct LFPulse : public Unit
{
	double mPhase;
	float mFreqMul, mDuty;
};

struct LFSaw : public Unit
{
	double mPhase;
	float mFreqMul;
};

struct LFPar : public Unit
{
	double mPhase;
	float mFreqMul;
};

struct LFCub : public Unit
{
	double mPhase;
	float mFreqMul;
};

struct LFTri : public Unit
{
	double mPhase;
	float mFreqMul;
};

struct Impulse : public Unit
{
	double mPhase, mPhaseOffset;
	float mFreqMul;
};

struct VarSaw : public Unit
{
	double mPhase;
	float mFreqMul, mDuty, mInvDuty, mInv1Duty;
};

struct SyncSaw : public Unit
{
	double mPhase1, mPhase2;
	float mFreqMul;
};

struct Line : public Unit
{
	double mLevel, mSlope;
	float mEndLevel;
	int mCounter;
};

struct XLine : public Unit
{
	double mLevel, mGrowth;
	float mEndLevel;
	int mCounter;
};

struct Cutoff : public Unit
{
	double mLevel, mSlope;
	int mWaitCounter;
};

struct LinExp : public Unit
{
	float m_dstratio, m_rsrcrange, m_rrminuslo, m_dstlo;
};

struct LinLin : public Unit
{
	float m_scale, m_offset;
};

struct Clip : public Unit
{
	float m_lo, m_hi;
};

struct Wrap : public Unit
{
	float m_lo, m_hi, m_range;
};

struct Fold : public Unit
{
	float m_lo, m_hi, m_range, m_range2;
};

struct Unwrap : public Unit
{
	float m_range, m_half, m_offset, m_prev;
};

struct InRange : public Unit
{
	// nothing
};

struct InRect : public Unit
{
	// nothing
};

struct Trapezoid : public Unit
{
	float m_leftScale, m_rightScale, m_a, m_b, m_c, m_d;
};

struct K2A : public Unit
{
	float mLevel;
};

struct Silent : public Unit
{
};

struct EnvGen : public Unit
{
	double m_a1, m_a2, m_b1, m_y1, m_y2, m_grow, m_level, m_endLevel;
	int m_counter, m_stage, m_shape, m_releaseNode;
	float m_prevGate;
	bool m_released;
};

struct BufEnvGen : public EnvGen
{
	SndBuf *m_buf;
	float m_fbufnum;
};

struct Linen : public Unit
{
	float m_endLevel;
	double m_slope, m_level;
	int m_counter, m_stage;
	float m_prevGate;
};

struct ADSR : public Unit
{
	double m_a2, m_b1, m_grow, m_level, m_endLevel;
	int m_counter, m_stage;
	float m_prevGate;
};

//////////////////////////////////////////////////////////////////////////////////////////////////


extern "C"
{
	void load(InterfaceTable *inTable);

	void Vibrato_next(Vibrato *unit, int inNumSamples);
	void Vibrato_Ctor(Vibrato* unit);

	void LFPulse_next_a(LFPulse *unit, int inNumSamples);
	void LFPulse_next_k(LFPulse *unit, int inNumSamples);
	void LFPulse_Ctor(LFPulse* unit);

	void LFSaw_next_a(LFSaw *unit, int inNumSamples);
	void LFSaw_next_k(LFSaw *unit, int inNumSamples);
	void LFSaw_Ctor(LFSaw* unit);

	void LFTri_next_a(LFTri *unit, int inNumSamples);
	void LFTri_next_k(LFTri *unit, int inNumSamples);
	void LFTri_Ctor(LFTri* unit);

	void LFPar_next_a(LFPar *unit, int inNumSamples);
	void LFPar_next_k(LFPar *unit, int inNumSamples);
	void LFPar_Ctor(LFPar* unit);

	void LFCub_next_a(LFCub *unit, int inNumSamples);
	void LFCub_next_k(LFCub *unit, int inNumSamples);
	void LFCub_Ctor(LFCub* unit);

	void VarSaw_next_a(VarSaw *unit, int inNumSamples);
	void VarSaw_next_k(VarSaw *unit, int inNumSamples);
	void VarSaw_Ctor(VarSaw* unit);

	void Impulse_next_a(Impulse *unit, int inNumSamples);
	void Impulse_next_kk(Impulse *unit, int inNumSamples);
	void Impulse_next_k(Impulse *unit, int inNumSamples);
	void Impulse_Ctor(Impulse* unit);

	void SyncSaw_next_aa(SyncSaw *unit, int inNumSamples);
	void SyncSaw_next_ak(SyncSaw *unit, int inNumSamples);
	void SyncSaw_next_ka(SyncSaw *unit, int inNumSamples);
	void SyncSaw_next_kk(SyncSaw *unit, int inNumSamples);
	void SyncSaw_Ctor(SyncSaw* unit);

	void K2A_next(K2A *unit, int inNumSamples);
	void K2A_Ctor(K2A* unit);

	void Silent_next(Silent *unit, int inNumSamples);
	void Silent_Ctor(Silent* unit);

	void Line_next(Line *unit, int inNumSamples);
	void Line_Ctor(Line* unit);

	void XLine_next(XLine *unit, int inNumSamples);
	void XLine_Ctor(XLine* unit);

	void Wrap_next(Wrap *unit, int inNumSamples);
	void Wrap_Ctor(Wrap* unit);

	void Fold_next(Fold *unit, int inNumSamples);
	void Fold_Ctor(Fold* unit);

	void Clip_next(Clip *unit, int inNumSamples);
	void Clip_Ctor(Clip* unit);

	void Unwrap_next(Unwrap* unit, int inNumSamples);
	void Unwrap_Ctor(Unwrap* unit);

	void InRange_next(InRange *unit, int inNumSamples);
	void InRange_Ctor(InRange* unit);

	void InRect_next(InRect *unit, int inNumSamples);
	void InRect_Ctor(InRect* unit);

	void LinExp_next(LinExp *unit, int inNumSamples);
	void LinExp_Ctor(LinExp* unit);

	void LinLin_next(LinLin *unit, int inNumSamples);
	void LinLin_Ctor(LinLin* unit);

	void EnvGen_next_k(EnvGen *unit, int inNumSamples);
	void EnvGen_next_aa(EnvGen *unit, int inNumSamples);
	void EnvGen_next_ak(EnvGen *unit, int inNumSamples);
	void EnvGen_Ctor(EnvGen *unit);

	void BufEnvGen_next_k(BufEnvGen *unit, int inNumSamples);
	void BufEnvGen_next_aa(BufEnvGen *unit, int inNumSamples);
	void BufEnvGen_next_ak(BufEnvGen *unit, int inNumSamples);
	void BufEnvGen_Ctor(BufEnvGen *unit);

	void Linen_next_k(Linen *unit, int inNumSamples);
	void Linen_Ctor(Linen *unit);

	void ADSR_next_k(ADSR *unit, int inNumSamples);
	void ADSR_Ctor(ADSR *unit);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

// in, rate, depth, rateVariation, depthVariation
// 0   1     2      3              4

void Vibrato_next(Vibrato *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	
	double ffreq = unit->mFreq;
	double phase = unit->mPhase;
	float scaleA = unit->m_scaleA;
	float scaleB = unit->m_scaleB;
	if (unit->m_delay > 0) 
	{
		int remain = sc_min(inNumSamples, unit->m_delay);
		unit->m_delay -= remain;
		inNumSamples -= remain;
		LOOP(remain, 
			ZXP(out) = ZXP(in);
		);
		if (unit->m_delay <= 0 && inNumSamples > 0) {
			if (unit->m_attack > 0) goto doAttack;
			else goto doNormal;
		}
	}
	else if (unit->m_attack) 
	{
doAttack:
		int remain = sc_min(inNumSamples, unit->m_attack);
		unit->m_attack -= remain;
		inNumSamples -= remain;
		double attackSlope = unit->m_attackSlope;
		double attackLevel = unit->m_attackLevel;
		LOOP(remain, 
			if (phase < 1.f) 
			{
				float z = phase;
				ZXP(out) = ZXP(in) * (1.f + (float)attackLevel * scaleA * (1.f - z * z)) ;
			} 
			else if (phase < 3.f) 
			{
				float z = phase - 2.f;
				ZXP(out) = ZXP(in) * (1.f + (float)attackLevel * scaleB * (z * z - 1.f)) ;
			} 
			else 
			{
				phase -= 4.f;
				float z = phase;
				
				float depth = ZIN0(2);
				float rateVariation = ZIN0(5);
				float depthVariation = ZIN0(6);
				
				float rate = ZIN0(1) * unit->mFreqMul;
				RGen& rgen = *unit->mParent->mRGen;
				ffreq  = rate  * (1.f + rateVariation  * rgen.frand2());
				scaleA = depth * (1.f + depthVariation * rgen.frand2());
				scaleB = depth * (1.f + depthVariation * rgen.frand2());
	
				ZXP(out) = ZXP(in) * (1.f + (float)attackLevel * scaleA * (1.f - z * z)) ;
			}
			phase += ffreq;
			attackLevel += attackSlope;
		);
		unit->m_attackLevel = attackLevel;
		if (unit->m_attack <= 0 && inNumSamples > 0) goto doNormal;
	}
	else 
	{
doNormal:
		LOOP(inNumSamples, 
			if (phase < 1.f) 
			{
				float z = phase;
				ZXP(out) = ZXP(in) * (1.f + scaleA * (1.f - z * z)) ;
			} 
			else if (phase < 3.f) 
			{
				float z = phase - 2.f;
				ZXP(out) = ZXP(in) * (1.f + scaleB * (z * z - 1.f)) ;
			} 
			else 
			{
				phase -= 4.f;
				float z = phase;
				
				float depth = ZIN0(2);
				float rateVariation = ZIN0(5);
				float depthVariation = ZIN0(6);
				
				float rate = ZIN0(1) * unit->mFreqMul;
				RGen& rgen = *unit->mParent->mRGen;
				ffreq  = rate  * (1.f + rateVariation  * rgen.frand2());
				scaleA = depth * (1.f + depthVariation * rgen.frand2());
				scaleB = depth * (1.f + depthVariation * rgen.frand2());
	
				ZXP(out) = ZXP(in) * (1.f + scaleA * (1.f - z * z)) ;
			}
			phase += ffreq;
		);
	}
	unit->mPhase = phase;
	unit->mFreq = ffreq;
	unit->m_scaleA = scaleA;
	unit->m_scaleB = scaleB;
	
}

void Vibrato_Ctor(Vibrato* unit)
{	
	unit->mFreqMul = 4.0 * SAMPLEDUR;
	unit->mPhase = 4.0 * sc_wrap(ZIN0(7), 0.f, 1.f) - 1.0;

	RGen& rgen = *unit->mParent->mRGen;
	float rate = ZIN0(1) * unit->mFreqMul;
	float depth = ZIN0(2);
	float rateVariation = ZIN0(5);
	float depthVariation = ZIN0(6);
	unit->mFreq    = rate  * (1.f + rateVariation  * rgen.frand2());
	unit->m_scaleA = depth * (1.f + depthVariation * rgen.frand2());
	unit->m_scaleB = depth * (1.f + depthVariation * rgen.frand2());
	unit->m_delay = (int)(ZIN0(3) * SAMPLERATE);
	unit->m_attack = (int)(ZIN0(4) * SAMPLERATE);
	unit->m_attackSlope = 1. / (double)(1 + unit->m_attack);
	unit->m_attackLevel = unit->m_attackSlope;
	
	SETCALC(Vibrato_next);
	Vibrato_next(unit, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////


void LFPulse_next_a(LFPulse *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	float nextDuty = ZIN0(2);
	float duty = unit->mDuty;
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z;
		if (phase >= 1.f) {
			phase -= 1.f;
			duty = unit->mDuty = nextDuty;
			// output at least one sample from the opposite polarity
			z = duty < 0.5 ? 1.f : 0.f;
		} else {
			z = phase < duty ? 1.f : 0.f;
		}
		phase += ZXP(freq) * freqmul;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
}

void LFPulse_next_k(LFPulse *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	float nextDuty = ZIN0(2);
	float duty = unit->mDuty;
	
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z;
		if (phase >= 1.f) {
			phase -= 1.f;
			duty = unit->mDuty = nextDuty;
			// output at least one sample from the opposite polarity
			z = duty < 0.5 ? 1.f : 0.f;
		} else {
			z = phase < duty ? 1.f : 0.f;
		}
		phase += freq;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
}

void LFPulse_Ctor(LFPulse* unit)
{
	if (INRATE(0) == calc_FullRate) {
		SETCALC(LFPulse_next_a);
	} else {
		SETCALC(LFPulse_next_k);
	}

	unit->mFreqMul = unit->mRate->mSampleDur;
	unit->mPhase = ZIN0(1);
	unit->mDuty = ZIN0(2);
	
	LFPulse_next_k(unit, 1);

}


//////////////////////////////////////////////////////////////////////////////////////////////////

void LFSaw_next_a(LFSaw *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z = phase; // out must be written last for in place operation
		phase += ZXP(freq) * freqmul;
		if (phase >= 1.f) phase -= 2.f;
		else if (phase <= -1.f) phase += 2.f;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
	
}

void LFSaw_next_k(LFSaw *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	
	double phase = unit->mPhase;
	if (freq >= 0.f) {
		LOOP(inNumSamples, 
			ZXP(out) = phase;
			phase += freq;
			if (phase >= 1.f) phase -= 2.f;
		);
	} else {
		LOOP(inNumSamples, 
			ZXP(out) = phase;
			phase += freq;
			if (phase <= -1.f) phase += 2.f;
		);
	}

	unit->mPhase = phase;
}

void LFSaw_Ctor(LFSaw* unit)
{
	if (INRATE(0) == calc_FullRate) {
		SETCALC(LFSaw_next_a);
	} else {
		SETCALC(LFSaw_next_k);
	}

	unit->mFreqMul = 2.0 * unit->mRate->mSampleDur;
	unit->mPhase = ZIN0(1);
	
	LFSaw_next_k(unit, 1);

}



//////////////////////////////////////////////////////////////////////////////////////////////////

void LFPar_next_a(LFPar *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		if (phase < 1.f) {
			float z = phase;
			ZXP(out) = 1.f - z*z;
		} else if (phase < 3.f) {
			float z = phase - 2.f;
			ZXP(out) = z*z - 1.f;
		} else {
			phase -= 4.f;
			float z = phase;
			ZXP(out) = 1.f - z*z;
		}
		phase += ZXP(freq) * freqmul;
	);

	unit->mPhase = phase;
	
}

void LFPar_next_k(LFPar *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		if (phase < 1.f) {
			float z = phase;
			ZXP(out) = 1.f - z*z;
		} else if (phase < 3.f) {
			float z = phase - 2.f;
			ZXP(out) = z*z - 1.f;
		} else {
			phase -= 4.f;
			float z = phase;
			ZXP(out) = 1.f - z*z;
		}
		phase += freq;
	);

	unit->mPhase = phase;
}

void LFPar_Ctor(LFPar* unit)
{
	if (INRATE(0) == calc_FullRate) {
		SETCALC(LFPar_next_a);
	} else {
		SETCALC(LFPar_next_k);
	}

	unit->mFreqMul = 4.0 * unit->mRate->mSampleDur;
	unit->mPhase = ZIN0(1);
	
	LFPar_next_k(unit, 1);

}



//////////////////////////////////////////////////////////////////////////////////////////////////

void LFCub_next_a(LFCub *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z;
		if (phase < 1.f) {
			z = phase;
		} else if (phase < 2.f) {
			z = 2.f - phase;
		} else {
			phase -= 2.f;
			z = phase;
		}
		ZXP(out) = z * z * (6.f - 4.f * z) - 1.f;
		phase += ZXP(freq) * freqmul;
	);

	unit->mPhase = phase;
	
}

void LFCub_next_k(LFCub *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z;
		if (phase < 1.f) {
			z = phase;
		} else if (phase < 2.f) {
			z = 2.f - phase;
		} else {
			phase -= 2.f;
			z = phase;
		}
		ZXP(out) = z * z * (6.f - 4.f * z) - 1.f;
		phase += freq;
	);

	unit->mPhase = phase;
}

void LFCub_Ctor(LFCub* unit)
{
	if (INRATE(0) == calc_FullRate) {
		SETCALC(LFCub_next_a);
	} else {
		SETCALC(LFCub_next_k);
	}

	unit->mFreqMul = 2.0 * unit->mRate->mSampleDur;
	unit->mPhase = ZIN0(1) + 0.5;
	
	LFCub_next_k(unit, 1);

}



//////////////////////////////////////////////////////////////////////////////////////////////////

void LFTri_next_a(LFTri *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z = phase > 1.f ? 2.f - phase : phase;
		phase += ZXP(freq) * freqmul;
		if (phase >= 3.f) phase -= 4.f;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
	
}

void LFTri_next_k(LFTri *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z = phase > 1.f ? 2.f - phase : phase;
		phase += freq;
		if (phase >= 3.f) phase -= 4.f;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
}

void LFTri_Ctor(LFTri* unit)
{
	if (INRATE(0) == calc_FullRate) {
		SETCALC(LFTri_next_a);
	} else {
		SETCALC(LFTri_next_k);
	}

	unit->mFreqMul = 4.0 * unit->mRate->mSampleDur;
	unit->mPhase = ZIN0(1);
	
	LFTri_next_k(unit, 1);

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Impulse_next_a(Impulse *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z;
		if (phase >= 1.f) {
			phase -= 1.f;
			z = 1.f;
		} else {
			z = 0.f;
		}
		phase += ZXP(freq) * freqmul;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
	
}

/* phase mod - jrh 03 */

void Impulse_next_ak(Impulse *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	double phaseOffset =  ZIN0(1);
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	double prev_phaseOffset = unit->mPhaseOffset;
	double phaseSlope = CALCSLOPE(phaseOffset, prev_phaseOffset);
	phase += prev_phaseOffset;
	
	LOOP(inNumSamples, 
		float z;
		phase += phaseSlope;
		if (phase >= 1.f) {
			phase -= 1.f;
			z = 1.f;
		} else {
			z = 0.f;
		}
		phase += ZXP(freq) * freqmul;
		ZXP(out) = z;
	);

	unit->mPhase = phase - phaseOffset;
	unit->mPhaseOffset = phaseOffset;
}

void Impulse_next_kk(Impulse *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	double phaseOffset =  ZIN0(1);
	
	double phase = unit->mPhase;
	double prev_phaseOffset = unit->mPhaseOffset;
	double phaseSlope = CALCSLOPE(phaseOffset, prev_phaseOffset);
	phase += prev_phaseOffset;
	
	LOOP(inNumSamples, 
		float z;
		phase += phaseSlope;
		if (phase >= 1.f) {
			phase -= 1.f;
			z = 1.f;
		} else {
			z = 0.f;
		}
		phase += freq;
		ZXP(out) = z;
	);

	unit->mPhase = phase - phaseOffset;
	unit->mPhaseOffset = phaseOffset;
}


void Impulse_next_k(Impulse *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	
	double phase = unit->mPhase;
	LOOP(inNumSamples, 
		float z;
		if (phase >= 1.f) {
			phase -= 1.f;
			z = 1.f;
		} else {
			z = 0.f;
		}
		phase += freq;
		ZXP(out) = z;
	);

	unit->mPhase = phase;
}

void Impulse_Ctor(Impulse* unit)
{
	
	unit->mPhase = ZIN0(1);
	
	if (INRATE(0) == calc_FullRate) {
		if(INRATE(1) != calc_ScalarRate) {
			SETCALC(Impulse_next_ak);
			unit->mPhase = 1.f;
		} else {
			SETCALC(Impulse_next_a);
		}
	} else {
		if(INRATE(1) != calc_ScalarRate) {
			SETCALC(Impulse_next_kk);
			unit->mPhase = 1.f;
		} else {
			SETCALC(Impulse_next_k);
		}
	}
	
	
	unit->mPhaseOffset = 0.f;
	unit->mFreqMul = unit->mRate->mSampleDur;
	if (unit->mPhase == 0.f) unit->mPhase = 1.f;
	
	ZOUT0(0) = 0.f;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VarSaw_next_a(VarSaw *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *freq = ZIN(0);
	float nextDuty = ZIN0(2);
	float duty = unit->mDuty;
	float invduty = unit->mInvDuty;
	float inv1duty = unit->mInv1Duty;
	
	float freqmul = unit->mFreqMul;
	double phase = unit->mPhase;
	
	LOOP(inNumSamples, 
		if (phase >= 1.f) {
			phase -= 1.f;
			duty = unit->mDuty = sc_clip(nextDuty, 0.001, 0.999);
			invduty = unit->mInvDuty = 2.f / duty;
			inv1duty = unit->mInv1Duty = 2.f / (1.f - duty);
		}
		float z = phase < duty ? phase * invduty : (1.f - phase) * inv1duty;
		phase += ZXP(freq) * freqmul;
		ZXP(out) = z - 1.f;
	);

	unit->mPhase = phase;
	
}

void VarSaw_next_k(VarSaw *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq = ZIN0(0) * unit->mFreqMul;
	float nextDuty = ZIN0(2);
	float duty = unit->mDuty;
	float invduty = unit->mInvDuty;
	float inv1duty = unit->mInv1Duty;
	
	double phase = unit->mPhase;
	
	LOOP(inNumSamples, 
		if (phase >= 1.f) {
			phase -= 1.f;
			duty = unit->mDuty = sc_clip(nextDuty, 0.001, 0.999);
			invduty = unit->mInvDuty = 2.f / duty;
			inv1duty = unit->mInv1Duty = 2.f / (1.f - duty);
		}
		float z = phase < duty ? phase * invduty : (1.f - phase) * inv1duty;
		phase += freq;
		ZXP(out) = z - 1.f;
	);

	unit->mPhase = phase;
}

void VarSaw_Ctor(VarSaw* unit)
{
	if (INRATE(0) == calc_FullRate) {
		SETCALC(VarSaw_next_a);
	} else {
		SETCALC(VarSaw_next_k);
	}

	unit->mFreqMul = unit->mRate->mSampleDur;
	unit->mPhase = ZIN0(1);
	float duty = ZIN0(2);
	duty = unit->mDuty = sc_clip(duty, 0.001, 0.999);
	unit->mInvDuty = 2.f / duty;
	unit->mInv1Duty = 2.f / (1.f - duty);
	
	ZOUT0(0) = 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SyncSaw_next_aa(SyncSaw *unit, int inNumSamples)
{
	float freqmul = unit->mFreqMul;
	float *out = ZOUT(0);
	float *freq1 = ZIN(0);
	float *freq2 = ZIN(1);
	
	double phase1 = unit->mPhase1;
	double phase2 = unit->mPhase2;
	
	LOOP(inNumSamples, 
		float freq1x = ZXP(freq1) * freqmul;
		float freq2x = ZXP(freq2) * freqmul;
		float z = phase2;
		phase2 += freq2x;
		if (phase2 >= 1.f) phase2 -= 2.f;
		phase1 += freq1x;
		if (phase1 >= 1.f) {
			phase1 -= 2.f;
			phase2 = (phase1 + 1.f) * freq2x / freq1x - 1.f;
		}
		ZXP(out) = z;
	);

	unit->mPhase1 = phase1;
	unit->mPhase2 = phase2;
}

void SyncSaw_next_ak(SyncSaw *unit, int inNumSamples)
{
	float freqmul = unit->mFreqMul;
	float *out = ZOUT(0);
	float *freq1 = ZIN(0);
	float freq2x = ZIN0(1) * freqmul;
	
	double phase1 = unit->mPhase1;
	double phase2 = unit->mPhase2;
	
	LOOP(inNumSamples, 
		float freq1x = ZXP(freq1) * freqmul;
		float z = phase2;
		phase2 += freq2x;
		if (phase2 >= 1.f) phase2 -= 2.f;
		phase1 += freq1x;
		if (phase1 >= 1.f) {
			phase1 -= 2.f;
			phase2 = (phase1 + 1.f) * freq2x / freq1x - 1.f;
		}
		ZXP(out) = z;
	);

	unit->mPhase1 = phase1;
	unit->mPhase2 = phase2;
}

void SyncSaw_next_ka(SyncSaw *unit, int inNumSamples)
{
	float freqmul = unit->mFreqMul;
	float *out = ZOUT(0);
	float freq1x = ZIN0(0) * freqmul;
	float *freq2 = ZIN(1);
	
	double phase1 = unit->mPhase1;
	double phase2 = unit->mPhase2;
	
	LOOP(inNumSamples, 
		float freq2x = ZXP(freq2) * freqmul;
		float z = phase2;
		phase2 += freq2x;
		if (phase2 >= 1.f) phase2 -= 2.f;
		phase1 += freq1x;
		if (phase1 >= 1.f) {
			phase1 -= 2.f;
			phase2 = (phase1 + 1.f) * freq2x / freq1x - 1.f;
		}
		ZXP(out) = z;
	);

	unit->mPhase1 = phase1;
	unit->mPhase2 = phase2;
}

void SyncSaw_next_kk(SyncSaw *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float freq1x = ZIN0(0) * unit->mFreqMul;
	float freq2x = ZIN0(1) * unit->mFreqMul;
	double phase1 = unit->mPhase1;
	double phase2 = unit->mPhase2;
	
	LOOP(inNumSamples, 
		float z = phase2;
		phase2 += freq2x;
		if (phase2 >= 1.f) phase2 -= 2.f;
		phase1 += freq1x;
		if (phase1 >= 1.f) {
			phase1 -= 2.f;
			phase2 = (phase1 + 1.f) * freq2x / freq1x - 1.f;
		}
		ZXP(out) = z;
	);

	unit->mPhase1 = phase1;
	unit->mPhase2 = phase2;

}

void SyncSaw_Ctor(SyncSaw* unit)
{
	if (INRATE(0) == calc_FullRate) {
		if (INRATE(1) == calc_FullRate) {
			SETCALC(SyncSaw_next_aa);
		} else {
			SETCALC(SyncSaw_next_ak);
		}
	} else {
		if (INRATE(1) == calc_FullRate) {
			SETCALC(SyncSaw_next_ka);
		} else {
			SETCALC(SyncSaw_next_kk);
		}
	}
	unit->mFreqMul = 2.0 * unit->mRate->mSampleDur;
	unit->mPhase1 = 0.;
	unit->mPhase2 = 0.;
	
	SyncSaw_next_kk(unit, 1);

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void K2A_next(K2A *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float in = ZIN0(0);
	
	float level = unit->mLevel;
	float slope = CALCSLOPE(in, level);
	
	LOOP(inNumSamples, 
		ZXP(out) = level += slope;
	);
	unit->mLevel = level;
}

void K2A_Ctor(K2A* unit)
{
	SETCALC(K2A_next);
	unit->mLevel = ZIN0(0);
	
	ZOUT0(0) = unit->mLevel;

}
//////////////////////////////////////////////////////////////////////////////////////////////////

void Silent_Ctor(Unit* unit)
{
	SETCALC(ClearUnitOutputs);
	ZOUT0(0) = 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Line_next(Line *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	
	double slope = unit->mSlope;
	double level = unit->mLevel;
	int counter = unit->mCounter;
	
	int remain = inNumSamples;
	do {
		if (counter==0) {
			int nsmps = remain;
			remain = 0;
			float endlevel = unit->mEndLevel;
			LOOP(nsmps, 
				ZXP(out) = endlevel;
			);
		} else {
			int nsmps = sc_min(remain, counter);
			counter -= nsmps;
			remain -= nsmps;
			LOOP(nsmps, 
				ZXP(out) = level;
				level += slope;
			);
			if (counter == 0) {
				unit->mDone = true;
				int doneAction = (int)ZIN0(3);
				DoneAction(doneAction, unit);
			}
		}
	} while (remain);
	unit->mCounter = counter;
	unit->mLevel = level;

}

void Line_Ctor(Line* unit)
{
	SETCALC(Line_next);
	double start = ZIN0(0);
	double end = ZIN0(1);
	double dur = ZIN0(2);
	
	int counter = (int)(dur * unit->mRate->mSampleRate + .5f);
	unit->mCounter = sc_max(1, counter);
	unit->mSlope = (end - start) / (counter);
	unit->mLevel = start;
	unit->mEndLevel = end;
	
	ZOUT0(0) = unit->mLevel;
	unit->mLevel += unit->mSlope;

}

//////////////////////////////////////////////////////////////////////////////////////////////////


void XLine_next(XLine *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	
	double grow = unit->mGrowth;
	double level = unit->mLevel;
	int counter = unit->mCounter;
	
	int remain = inNumSamples;
	do {
		if (counter==0) {
			int nsmps = remain;
			remain = 0;
			float endlevel = unit->mEndLevel;
			LOOP(nsmps, 
				ZXP(out) = endlevel;
			);
		} else {
			int nsmps = sc_min(remain, counter);
			counter -= nsmps;
			remain -= nsmps;
			LOOP(nsmps, 
				ZXP(out) = level;
				level *= grow;
			);
			if (counter == 0) {
				unit->mDone = true;
				int doneAction = (int)ZIN0(3);
				DoneAction(doneAction, unit);
			}
		}
	} while (remain);
	unit->mCounter = counter;
	unit->mLevel = level;
}

void XLine_Ctor(XLine* unit)
{
	SETCALC(XLine_next);
	double start = ZIN0(0);
	double end = ZIN0(1);
	double dur = ZIN0(2);
	
	int counter = (int)(dur * unit->mRate->mSampleRate + .5f);
	unit->mCounter = sc_max(1, counter);
	unit->mGrowth = pow(end / start, 1.0 / counter);
	unit->mLevel = start;
	unit->mEndLevel = end;
	
	ZOUT0(0) = unit->mLevel;
	unit->mLevel *= unit->mGrowth;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Wrap_next(Wrap* unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	float lo = unit->m_lo;
	float hi = unit->m_hi;
	float range = unit->m_range;
	
	LOOP(inNumSamples, 
		ZXP(out) = sc_wrap(ZXP(in), lo, hi, range);
	);
}

void Wrap_Ctor(Wrap* unit)
{

	SETCALC(Wrap_next);
	unit->m_lo = ZIN0(1);
	unit->m_hi = ZIN0(2);
	
	if (unit->m_lo > unit->m_hi) {
		float temp = unit->m_lo;
		unit->m_lo = unit->m_hi;
		unit->m_hi = temp;
	}
	unit->m_range = unit->m_hi - unit->m_lo;
	
	Wrap_next(unit, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Fold_next(Fold* unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	float lo = unit->m_lo;
	float hi = unit->m_hi;
	float range = unit->m_range;
	float range2 = unit->m_range2;
	
	LOOP(inNumSamples, 
		ZXP(out) = sc_fold(ZXP(in), lo, hi, range, range2);
	);
}

void Fold_Ctor(Fold* unit)
{

	SETCALC(Fold_next);
	unit->m_lo = ZIN0(1);
	unit->m_hi = ZIN0(2);
	
	if (unit->m_lo > unit->m_hi) {
		float temp = unit->m_lo;
		unit->m_lo = unit->m_hi;
		unit->m_hi = temp;
	}
	unit->m_range = unit->m_hi - unit->m_lo;
	unit->m_range2 = 2.f * unit->m_range;
	
	Fold_next(unit, 1);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void Clip_next(Clip* unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	float lo = unit->m_lo;
	float hi = unit->m_hi;
	
	LOOP(inNumSamples, 
		ZXP(out) = sc_clip(ZXP(in), lo, hi);
	);
}

void Clip_Ctor(Clip* unit)
{

	SETCALC(Clip_next);
	unit->m_lo = ZIN0(1);
	unit->m_hi = ZIN0(2);
	
	if (unit->m_lo > unit->m_hi) {
		float temp = unit->m_lo;
		unit->m_lo = unit->m_hi;
		unit->m_hi = temp;
	}
	
	Clip_next(unit, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Unwrap_next(Unwrap* unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	float range = unit->m_range;
	float half = unit->m_half;
	float prev = unit->m_prev;
	float offset = unit->m_offset;
	
	LOOP(inNumSamples, 
		float zin = ZXP(in);
		float diff = zin - prev;
		if (fabs(diff) > half) {
			if (zin < prev) offset += range;
			else offset -= range;
		}
		ZXP(out) = zin + offset;
		prev = zin;
	);
	unit->m_prev = prev;
	unit->m_offset = offset;
}

void Unwrap_Ctor(Unwrap* unit)
{

	SETCALC(Unwrap_next);
	float in   = ZIN0(0);
	float lo = ZIN0(1);
	float hi = ZIN0(2);
	
	if (lo > hi) {
		float temp = lo;
		lo = hi;
		hi = temp;
	}
	unit->m_range = fabs(hi - lo);
	unit->m_half = unit->m_range * 0.5f;
	
	if (in < lo || in >= hi) unit->m_offset = floor((lo - in)/unit->m_range) * unit->m_range;
	else unit->m_offset = 0.f;
	
	Unwrap_next(unit, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InRange_next(InRange *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	float lo   = ZIN0(1);
	float hi   = ZIN0(2);
	
	LOOP(inNumSamples, 
		float zin = ZXP(in);
		ZXP(out) = zin >= lo && zin <= hi ? 1.f : 0.f;
	);
}

void InRange_Ctor(InRange* unit)
{
	SETCALC(InRange_next);
	InRange_next(unit, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InRect_next(InRect* unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *inx   = ZIN(0);
	float *iny   = ZIN(1);
	float left   = ZIN0(2);
	float top    = ZIN0(3);
	float right  = ZIN0(4);
	float bottom = ZIN0(5);
	
	LOOP(inNumSamples, 
		float x = ZXP(inx);
		float y = ZXP(iny);
		ZXP(out) = x >= left && x <= right && y >= top && y <= bottom ? 1.f : 0.f;
	);
}

void InRect_Ctor(InRect* unit)
{
	SETCALC(InRect_next);
	InRect_next(unit, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void LinExp_next(LinExp *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	float dstlo = unit->m_dstlo;
	
	float dstratio = unit->m_dstratio;
	float rsrcrange = unit->m_rsrcrange;
	float rrminuslo = unit->m_rrminuslo;

	LOOP(inNumSamples, 
		ZXP(out) = dstlo * pow(dstratio, ZXP(in) * rsrcrange + rrminuslo);
	);
}

void LinExp_Ctor(LinExp* unit)
{
	SETCALC(LinExp_next);

	float srclo = ZIN0(1);
	float srchi = ZIN0(2);
	float dstlo = ZIN0(3);
	float dsthi = ZIN0(4);
	unit->m_dstlo = dstlo;
	unit->m_dstratio = dsthi/dstlo;
	unit->m_rsrcrange = 1. / (srchi - srclo);	
	unit->m_rrminuslo = unit->m_rsrcrange * -srclo;

	LinExp_next(unit, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void LinLin_next(LinLin *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in   = ZIN(0);
	
	float scale = unit->m_scale;
	float offset = unit->m_offset;

	LOOP(inNumSamples, 
		ZXP(out) = scale * ZXP(in) + offset;
	);
}

void LinLin_Ctor(LinLin* unit)
{
	SETCALC(LinLin_next);

	float srclo = ZIN0(1);
	float srchi = ZIN0(2);
	float dstlo = ZIN0(3);
	float dsthi = ZIN0(4);
	
	unit->m_scale = (dsthi - dstlo) / (srchi - srclo);
	unit->m_offset = dstlo - unit->m_scale * srclo;

	LinLin_next(unit, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////


enum {
	kEnvGen_gate,
	kEnvGen_levelScale,
	kEnvGen_levelBias,
	kEnvGen_timeScale,
	kEnvGen_doneAction,
	kEnvGen_initLevel,
	kEnvGen_numStages,
	kEnvGen_releaseNode,
	kEnvGen_loopNode
};

void EnvGen_Ctor(EnvGen *unit)
{
	//Print("EnvGen_Ctor A\n");
	if (unit->mCalcRate == calc_FullRate) {
		if (INRATE(1) == calc_FullRate) {
			SETCALC(EnvGen_next_aa);
		} else {
			SETCALC(EnvGen_next_ak);
		}
	} else {
		SETCALC(EnvGen_next_k);
	}

	// gate = 1.0, levelScale = 1.0, levelBias = 0.0, timeScale
	// level0, numstages, releaseNode, loopNode,
	// [level, dur, shape, curve]
		
	unit->m_endLevel = unit->m_level = ZIN0(kEnvGen_initLevel);
	unit->m_counter = 0;
	unit->m_stage = 1000000000;
	unit->m_prevGate = 0.f;
	unit->m_released = false;
	unit->m_releaseNode = (int)ZIN0(kEnvGen_releaseNode);
	EnvGen_next_k(unit, 1);
}

enum {
	shape_Step,
	shape_Linear,
	shape_Exponential,
	shape_Sine,
	shape_Welch,
	shape_Curve,
	shape_Squared,
	shape_Cubed,
	shape_Sustain = 9999
};

void EnvGen_next_k(EnvGen *unit, int inNumSamples)
{
	float *out = OUT(0);
	float gate = ZIN0(kEnvGen_gate); 
	//Print("->EnvGen_next_k gate %g\n", gate);
	int counter = unit->m_counter;
	double level = unit->m_level;

	if (unit->m_prevGate <= 0. && gate > 0.) {
		unit->m_stage = -1;
		unit->mDone = false;
		unit->m_released = false;
		counter = 0;
	} else if (gate <= -1.f && unit->m_prevGate > -1.f) {
		// cutoff
		int numstages = (int)ZIN0(kEnvGen_numStages);
		float dur = -gate - 1.f;
		counter  = (int32)(dur * SAMPLERATE);
		counter  = sc_max(1, counter);
		unit->m_stage = numstages;
		unit->m_shape = shape_Linear;
		unit->m_grow = -level / counter;
		unit->m_endLevel = 0.;
	} else if (unit->m_prevGate > 0.f && gate <= 0.f 
			&& unit->m_releaseNode >= 0 && !unit->m_released) {
		counter = 0;
		unit->m_stage = unit->m_releaseNode - 1;
		unit->m_released = true;
	}
	unit->m_prevGate = gate;
	

	// gate = 1.0, levelScale = 1.0, levelBias = 0.0, timeScale
	// level0, numstages, releaseNode, loopNode,
	// [level, dur, shape, curve]

	if (counter <= 0) {
		//Print("stage %d rel %d\n", unit->m_stage, (int)ZIN0(kEnvGen_releaseNode));
		int numstages = (int)ZIN0(kEnvGen_numStages);
		
		//Print("stage %d   numstages %d\n", unit->m_stage, numstages);
		if (unit->m_stage+1 >= numstages) { // num stages
		//Print("stage+1 > num stages\n");
			counter = INT_MAX;
			unit->m_shape = 0;
			level = unit->m_endLevel;
			unit->mDone = true;
			int doneAction = (int)ZIN0(kEnvGen_doneAction);
			DoneAction(doneAction, unit);
		} else if (unit->m_stage+1 == unit->m_releaseNode && !unit->m_released) { // sustain stage
			int loopNode = (int)ZIN0(kEnvGen_loopNode);
			if (loopNode >= 0 && loopNode < numstages) {
				unit->m_stage = loopNode;
				goto initSegment;
			} else {
				counter = INT_MAX;
				unit->m_shape = shape_Sustain;
				level = unit->m_endLevel;
			}
		//Print("sustain\n");
		} else {
			unit->m_stage++;
	initSegment:
		//Print("stage %d\n", unit->m_stage);
		//Print("initSegment\n");
			//out = unit->m_level;
			int stageOffset = (unit->m_stage << 2) + 9;
			
			if (stageOffset + 4 > unit->mNumInputs) {
				// oops.
				Print("envelope went past end of inputs.\n");
				ClearUnitOutputs(unit, 1);
				NodeEnd(&unit->mParent->mNode);
				return;
			}

			float** envPtr  = unit->mInBuf + stageOffset;
			double endLevel = *envPtr[0] * ZIN0(kEnvGen_levelScale) + ZIN0(kEnvGen_levelBias); // scale levels
			double dur      = *envPtr[1] * ZIN0(kEnvGen_timeScale);
			unit->m_shape   = (int32)*envPtr[2];
			double curve    = *envPtr[3];
			unit->m_endLevel = endLevel;
					
			counter  = (int32)(dur * SAMPLERATE);
			counter  = sc_max(1, counter);
		//Print("stageOffset %d   level %g   endLevel %g   dur %g   shape %d   curve %g\n", stageOffset, level, endLevel, dur, unit->m_shape, curve);
		//Print("SAMPLERATE %g\n", SAMPLERATE);
			if (counter == 1) unit->m_shape = 1; // shape_Linear
		//Print("new counter = %d  shape = %d\n", counter, unit->m_shape);
			switch (unit->m_shape) {
				case shape_Step : {
					level = endLevel;
				} break;
				case shape_Linear : {
					unit->m_grow = (endLevel - level) / counter;
					//Print("grow %g\n", unit->m_grow);
				} break;
				case shape_Exponential : {
					unit->m_grow = pow(endLevel / level, 1.0 / counter);
				} break;
				case shape_Sine : {
					double w = pi / counter;
		
					unit->m_a2 = (endLevel + level) * 0.5;
					unit->m_b1 = 2. * cos(w);
					unit->m_y1 = (endLevel - level) * 0.5;
					unit->m_y2 = unit->m_y1 * sin(pi * 0.5 - w);
					level = unit->m_a2 - unit->m_y1;
				} break;
				case shape_Welch : {
					double w = (pi * 0.5) / counter;
					
					unit->m_b1 = 2. * cos(w);
					
					if (endLevel >= level) {
						unit->m_a2 = level;
						unit->m_y1 = 0.;
						unit->m_y2 = -sin(w) * (endLevel - level);
					} else {
						unit->m_a2 = endLevel;
						unit->m_y1 = level - endLevel;
						unit->m_y2 = cos(w) * (level - endLevel);
					}
					level = unit->m_a2 + unit->m_y1;
				} break;
				case shape_Curve : {
					if (fabs(curve) < 0.001) {
						unit->m_shape = 1; // shape_Linear
						unit->m_grow = (endLevel - level) / counter;
					} else {
						double a1 = (endLevel - level) / (1.0 - exp(curve));	
						unit->m_a2 = level + a1;
						unit->m_b1 = a1; 
						unit->m_grow = exp(curve / counter);
					}
				} break;
				case shape_Squared : {
					unit->m_y1 = sqrt(level); 
					unit->m_y2 = sqrt(endLevel); 
					unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
				} break;
				case shape_Cubed : {
					unit->m_y1 = pow(level, 0.33333333); 
					unit->m_y2 = pow(endLevel, 0.33333333); 
					unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
				} break;
			}
		}
	}

	*out = level;
	switch (unit->m_shape) {
		case shape_Step : {
		} break;
		case shape_Linear : {
			double grow = unit->m_grow;
					//Print("level %g\n", level);
				level += grow;
		} break;
		case shape_Exponential : {
			double grow = unit->m_grow;
				level *= grow;
		} break;
		case shape_Sine : {
			double a2 = unit->m_a2;
			double b1 = unit->m_b1;
			double y2 = unit->m_y2;
			double y1 = unit->m_y1;
				double y0 = b1 * y1 - y2; 
				level = a2 - y0;
				y2 = y1; 
				y1 = y0;
			unit->m_y1 = y1;
			unit->m_y2 = y2;
		} break;
		case shape_Welch : {
			double a2 = unit->m_a2;
			double b1 = unit->m_b1;
			double y2 = unit->m_y2;
			double y1 = unit->m_y1;
				double y0 = b1 * y1 - y2; 
				level = a2 - y0;
				y2 = y1; 
				y1 = y0;
			unit->m_y1 = y1;
			unit->m_y2 = y2;
		} break;
		case shape_Curve : {
			double a2 = unit->m_a2;
			double b1 = unit->m_b1;
			double grow = unit->m_grow;
				b1 *= grow;
				level = a2 - b1;
			unit->m_b1 = b1;
		} break;
			case shape_Squared : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
					y1 += grow;
					level = y1*y1;
				unit->m_y1 = y1;
			} break;
			case shape_Cubed : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
					y1 += grow;
					level = y1*y1*y1;
				unit->m_y1 = y1;
			} break;
		case shape_Sustain : {
		} break;
	}
	//Print("x %d %d %d %g\n", unit->m_stage, counter, unit->m_shape, *out);
	unit->m_level = level;
	unit->m_counter = counter - 1;

}

void EnvGen_next_ak(EnvGen *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float gate = ZIN0(kEnvGen_gate);
	int counter = unit->m_counter;
	double level = unit->m_level;
	
	if (unit->m_prevGate <= 0. && gate > 0.) {
		unit->m_stage = -1;
		unit->mDone = false;
		unit->m_released = false;
		counter = 0;
	} else if (gate <= -1.f && unit->m_prevGate > -1.f) {
		// cutoff
		int numstages = (int)ZIN0(kEnvGen_numStages);
		float dur = -gate - 1.f;
		counter  = (int32)(dur * SAMPLERATE);
		counter  = sc_max(1, counter);
		unit->m_stage = numstages;
		unit->m_shape = shape_Linear;
		unit->m_grow = -level / counter;
		unit->m_endLevel = 0.;
	} else if (unit->m_prevGate > 0.f && gate <= 0.f 
			&& unit->m_releaseNode >= 0 && !unit->m_released) {
		counter = 0;
		unit->m_stage = unit->m_releaseNode - 1;
		unit->m_released = true;
	}
	unit->m_prevGate = gate;
	
	int remain = inNumSamples;
	while (remain)
	{
		if (counter == 0) {
			int numstages = (int)ZIN0(kEnvGen_numStages);
			
			if (unit->m_stage+1 >= numstages) { // num stages
				counter = INT_MAX;
				unit->m_shape = 0;
				level = unit->m_endLevel;
				unit->mDone = true;
				int doneAction = (int)ZIN0(kEnvGen_doneAction);
				DoneAction(doneAction, unit);
			} else if (unit->m_stage+1 == (int)ZIN0(kEnvGen_releaseNode) && !unit->m_released) { // sustain stage
				int loopNode = (int)ZIN0(kEnvGen_loopNode);
				if (loopNode >= 0 && loopNode < numstages) {
					unit->m_stage = loopNode;
					goto initSegment;
				} else {
					counter = INT_MAX;
					unit->m_shape = shape_Sustain;
					level = unit->m_endLevel;
				}
			} else {
				unit->m_stage++;
	initSegment:
				int stageOffset = (unit->m_stage << 2) + 9;
				
				if (stageOffset + 4 > unit->mNumInputs) {
					// oops.
					Print("envelope went past end of inputs.\n");
					ClearUnitOutputs(unit, 1);
					NodeEnd(&unit->mParent->mNode);
					return;
				}

				float** envPtr  = unit->mInBuf + stageOffset;
				double endLevel = *envPtr[0] * ZIN0(kEnvGen_levelScale) + ZIN0(kEnvGen_levelBias); // scale levels
				double dur      = *envPtr[1] * ZIN0(kEnvGen_timeScale);
				unit->m_shape   = (int32)*envPtr[2];
				double curve    = *envPtr[3];
				unit->m_endLevel = endLevel;
						
				counter  = (int32)(dur * SAMPLERATE);
				counter  = sc_max(1, counter);

				if (counter == 1) unit->m_shape = 1; // shape_Linear
				switch (unit->m_shape) {
					case shape_Step : {
						level = endLevel;
					} break;
					case shape_Linear : {
						unit->m_grow = (endLevel - level) / counter;
					} break;
					case shape_Exponential : {
						unit->m_grow = pow(endLevel / level, 1.0 / counter);
					} break;
					case shape_Sine : {
						double w = pi / counter;
			
						unit->m_a2 = (endLevel + level) * 0.5;
						unit->m_b1 = 2. * cos(w);
						unit->m_y1 = (endLevel - level) * 0.5;
						unit->m_y2 = unit->m_y1 * sin(pi * 0.5 - w);
						level = unit->m_a2 - unit->m_y1;
					} break;
					case shape_Welch : {
						double w = (pi * 0.5) / counter;
						
						unit->m_b1 = 2. * cos(w);
						
						if (endLevel >= level) {
							unit->m_a2 = level;
							unit->m_y1 = 0.;
							unit->m_y2 = -sin(w) * (endLevel - level);
						} else {
							unit->m_a2 = endLevel;
							unit->m_y1 = level - endLevel;
							unit->m_y2 = cos(w) * (level - endLevel);
						}
						level = unit->m_a2 + unit->m_y1;
					} break;
					case shape_Curve : {
						if (fabs(curve) < 0.001) {
							unit->m_shape = 1; // shape_Linear
							unit->m_grow = (endLevel - level) / counter;
						} else {
							double a1 = (endLevel - level) / (1.0 - exp(curve));	
							unit->m_a2 = level + a1;
							unit->m_b1 = a1; 
							unit->m_grow = exp(curve / counter);
						}
					} break;
					case shape_Squared : {
						unit->m_y1 = sqrt(level); 
						unit->m_y2 = sqrt(endLevel); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
					case shape_Cubed : {
						unit->m_y1 = pow(level, 0.33333333); 
						unit->m_y2 = pow(endLevel, 0.33333333); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
				}
			}
		}

		int nsmps = sc_min(remain, counter);
		switch (unit->m_shape) {
			case shape_Step : {
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
				}
			} break;
			case shape_Linear : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					level += grow;
				}
			} break;
			case shape_Exponential : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					level *= grow;
				}
			} break;
			case shape_Sine : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Welch : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Curve : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					b1 *= grow;
					level = a2 - b1;
				}
				unit->m_b1 = b1;
			} break;
			case shape_Squared : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Cubed : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Sustain : {
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
				}
			} break;
		}
		remain -= nsmps;
		counter -= nsmps;
	}
	//Print("x %d %d %d %g\n", unit->m_stage, counter, unit->m_shape, ZOUT0(0));
	unit->m_level = level;
	unit->m_counter = counter;

}

#define CHECK_GATE \
	gate = ZXP(gatein); \
	if (prevGate <= 0.f && gate > 0.f) { \
		unit->m_stage = -1; \
		unit->m_released = false; \
		unit->mDone = false; \
		counter = 0; \
		nsmps = i; \
		break; \
	} else if (gate <= -1.f && unit->m_prevGate > -1.f) { \
		int numstages = (int)ZIN0(kEnvGen_numStages); \
		float dur = -gate - 1.f; \
		counter  = (int32)(dur * SAMPLERATE); \
		counter  = sc_max(1, counter); \
		unit->m_stage = numstages; \
		unit->m_shape = shape_Linear; \
		unit->m_grow = -level / counter; \
		unit->m_endLevel = 0.; \
		nsmps = i; \
		break; \
	} else if (prevGate > 0.f && gate <= 0.f \
			&& unit->m_releaseNode >= 0 && !unit->m_released) { \
		counter = 0; \
		unit->m_stage = unit->m_releaseNode - 1; \
		unit->m_released = true; \
		nsmps = i; \
		break; \
	} \
	

void EnvGen_next_aa(EnvGen *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *gatein = ZIN(kEnvGen_gate);
	int counter = unit->m_counter;
	double level = unit->m_level;
	float gate = 0.;
	float prevGate = unit->m_prevGate;
	int remain = inNumSamples;
	while (remain)
	{
		if (counter == 0) {

			int numstages = (int)ZIN0(kEnvGen_numStages);
			
			if (unit->m_stage+1 >= numstages) { // num stages
				counter = INT_MAX;
				unit->m_shape = 0;
				level = unit->m_endLevel;
				unit->mDone = true;
				int doneAction = (int)ZIN0(kEnvGen_doneAction);
				DoneAction(doneAction, unit);
			} else if (unit->m_stage+1 == (int)ZIN0(kEnvGen_releaseNode) && !unit->m_released) { // sustain stage
				int loopNode = (int)ZIN0(kEnvGen_loopNode);
				if (loopNode >= 0 && loopNode < numstages) {
					unit->m_stage = loopNode;
					goto initSegment;
				} else {
					counter = INT_MAX;
					unit->m_shape = shape_Sustain;
					level = unit->m_endLevel;
				}
			} else {
				unit->m_stage++;
	initSegment:
				int stageOffset = (unit->m_stage << 2) + 9;
				
				if (stageOffset + 4 > unit->mNumInputs) {
					// oops.
					Print("envelope went past end of inputs.\n");
					ClearUnitOutputs(unit, 1);
					NodeEnd(&unit->mParent->mNode);
					return;
				}

				float** envPtr  = unit->mInBuf + stageOffset;
				double endLevel = *envPtr[0] * ZIN0(kEnvGen_levelScale) + ZIN0(kEnvGen_levelBias); // scale levels
				double dur      = *envPtr[1] * ZIN0(kEnvGen_timeScale);
				unit->m_shape   = (int32)*envPtr[2];
				double curve    = *envPtr[3];
				unit->m_endLevel = endLevel;
						
				counter  = (int32)(dur * SAMPLERATE);
				counter  = sc_max(1, counter);

				if (counter == 1) unit->m_shape = 1; // shape_Linear
				switch (unit->m_shape) {
					case shape_Step : {
						level = endLevel;
					} break;
					case shape_Linear : {
						unit->m_grow = (endLevel - level) / counter;
					} break;
					case shape_Exponential : {
						unit->m_grow = pow(endLevel / level, 1.0 / counter);
					} break;
					case shape_Sine : {
						double w = pi / counter;
			
						unit->m_a2 = (endLevel + level) * 0.5;
						unit->m_b1 = 2. * cos(w);
						unit->m_y1 = (endLevel - level) * 0.5;
						unit->m_y2 = unit->m_y1 * sin(pi * 0.5 - w);
						level = unit->m_a2 - unit->m_y1;
					} break;
					case shape_Welch : {
						double w = (pi * 0.5) / counter;
						
						unit->m_b1 = 2. * cos(w);
						
						if (endLevel >= level) {
							unit->m_a2 = level;
							unit->m_y1 = 0.;
							unit->m_y2 = -sin(w) * (endLevel - level);
						} else {
							unit->m_a2 = endLevel;
							unit->m_y1 = level - endLevel;
							unit->m_y2 = cos(w) * (level - endLevel);
						}
						level = unit->m_a2 + unit->m_y1;
					} break;
					case shape_Curve : {
						if (fabs(curve) < 0.001) {
							unit->m_shape = 1; // shape_Linear
							unit->m_grow = (endLevel - level) / counter;
						} else {
							double a1 = (endLevel - level) / (1.0 - exp(curve));	
							unit->m_a2 = level + a1;
							unit->m_b1 = a1; 
							unit->m_grow = exp(curve / counter);
						}
					} break;
					case shape_Squared : {
						unit->m_y1 = sqrt(level); 
						unit->m_y2 = sqrt(endLevel); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
					case shape_Cubed : {
						unit->m_y1 = pow(level, 0.33333333); 
						unit->m_y2 = pow(endLevel, 0.33333333); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
				}
			}
		}

		int nsmps = sc_min(remain, counter);
		switch (unit->m_shape) {
			case shape_Step : {
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
				}
			} break;
			case shape_Linear : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					level += grow;
				}
			} break;
			case shape_Exponential : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					level *= grow;
				}
			} break;
			case shape_Sine : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Welch : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Curve : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					b1 *= grow;
					level = a2 - b1;
				}
				unit->m_b1 = b1;
			} break;
			case shape_Squared : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Cubed : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Sustain : {
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
				}
			} break;
		}
		remain -= nsmps;
		counter -= nsmps;
	}
	unit->m_level = level;
	unit->m_counter = counter;
	unit->m_prevGate = gate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
void BufEnvGen_Ctor(BufEnvGen *unit)
{
	//Print("BufEnvGen_Ctor A\n");
	if (unit->mCalcRate == calc_FullRate) {
		if (INRATE(1) == calc_FullRate) {
			SETCALC(BufEnvGen_next_aa);
		} else {
			SETCALC(BufEnvGen_next_ak);
		}
	} else {
		SETCALC(BufEnvGen_next_k);
	}
	//Print("BufEnvGen_Ctor B\n");
	// get table
	float fbufnum = ZIN0(0);
	uint32 bufnum = (int)fbufnum;
	World *world = unit->mWorld;
	if (bufnum >= world->mNumSndBufs) bufnum = 0;
	unit->m_buf = world->mSndBufs + bufnum;
	SndBuf *buf = unit->m_buf;
	int tableSize = buf->samples;
	if (!buf || !buf->data || buf->samples < 8) {
		Print("Envelope not allocated.\n");
		//Print("bufnum %d\n", bufnum);
		//Print("buf %08X\n", buf);
		if (buf) {
			//Print("buf->data %08X   buf->samples %d\n", buf->data, buf->samples);
		}
		SETCALC(ClearUnitOutputs);
		return;
	}
	//Print("BufEnvGen_Ctor C\n");
	
	unit->m_level = buf->data[0];
	unit->m_counter = 0;
	unit->m_stage = -1;
	unit->m_prevGate = 0.f;
	unit->m_released = false;
	//Print("BufEnvGen_Ctor D\n");
	//ZOUT0(0) = unit->m_level;
	BufEnvGen_next_k(unit, 1);
}

void BufEnvGen_next_k(BufEnvGen *unit, int inNumSamples)
{
	float *out = OUT(0);
	float gate = ZIN0(1); 
	//Print("->BufEnvGen_next_k gate %g\n", gate);
	int counter = unit->m_counter;
	double level = unit->m_level;

	if (unit->m_prevGate <= 0. && gate > 0.) {
		unit->m_stage = -1;
		unit->mDone = false;
		unit->m_released = false;
		counter = 0;
	}
	unit->m_prevGate = gate;
	
	if (counter <= 0) {
		//Print("counter == 0\n");
		// get table
		float fbufnum = ZIN0(0);
		if (fbufnum != unit->m_fbufnum) {
			unit->m_fbufnum = fbufnum;
			uint32 bufnum = (int)fbufnum;
			World *world = unit->mWorld;
			if (bufnum >= world->mNumSndBufs) bufnum = 0;
			unit->m_buf = world->mSndBufs + bufnum;
		}
		SndBuf *buf = unit->m_buf;
		int tableSize = buf->samples;

		if (unit->m_stage+1 >= buf->data[1]) { // num stages
		//Print("stage > num stages\n");
			counter = INT_MAX;
			unit->m_shape = 0;
			level = unit->m_endLevel;
			unit->mDone = true;
		} else if (unit->m_stage == buf->data[2] && !unit->m_released) { // sustain stage
		//Print("sustain\n");
			counter = INT_MAX;
			unit->m_shape = shape_Sustain;
			level = unit->m_endLevel;
		} else {
			unit->m_stage++;
		//Print("stage %d\n", unit->m_stage);
		//Print("initSegment\n");
			//out = unit->m_level;
			int stageOffset = (unit->m_stage << 2) + 4;
			if (stageOffset + 4 > tableSize) {
				// oops.
				Print("envelope went past end of buffer.\n");
				ClearUnitOutputs(unit, 1);
				NodeEnd(&unit->mParent->mNode);
				return;
			}
			float* table = buf->data + stageOffset;
			double endLevel = table[0] * ZIN0(2) + ZIN0(3); // scale levels
			double dur      = table[1] * ZIN0(4);
			unit->m_shape   = (int32)table[2];
			double curve    = table[3];
			unit->m_endLevel = endLevel;
					
			counter  = (int32)(dur * SAMPLERATE);
			counter  = sc_max(1, counter);
		//Print("stageOffset %d   endLevel %g   dur %g   shape %d   curve %g\n", stageOffset, endLevel, dur, unit->m_shape, curve);
		//Print("SAMPLERATE %g\n", SAMPLERATE);
			if (counter == 1) unit->m_shape = 1; // shape_Linear
		//Print("new counter = %d  shape = %d\n", counter, unit->m_shape);
			switch (unit->m_shape) {
				case shape_Step : {
					level = endLevel;
				} break;
				case shape_Linear : {
					unit->m_grow = (endLevel - level) / counter;
					//Print("grow %g\n", unit->m_grow);
				} break;
				case shape_Exponential : {
					unit->m_grow = pow(endLevel / level, 1.0 / counter);
				} break;
				case shape_Sine : {
					double w = pi / counter;
		
					unit->m_a2 = (endLevel + level) * 0.5;
					unit->m_b1 = 2. * cos(w);
					unit->m_y1 = (endLevel - level) * 0.5;
					unit->m_y2 = unit->m_y1 * sin(pi * 0.5 - w);
					level = unit->m_a2 - unit->m_y1;
				} break;
				case shape_Welch : {
					double w = (pi * 0.5) / counter;
					
					unit->m_b1 = 2. * cos(w);
					
					if (endLevel >= level) {
						unit->m_a2 = level;
						unit->m_y1 = 0.;
						unit->m_y2 = -sin(w) * (endLevel - level);
					} else {
						unit->m_a2 = endLevel;
						unit->m_y1 = level - endLevel;
						unit->m_y2 = cos(w) * (level - endLevel);
					}
					level = unit->m_a2 + unit->m_y1;
				} break;
				case shape_Curve : {
					if (fabs(curve) < 0.001) {
						unit->m_shape = 1; // shape_Linear
						unit->m_grow = (endLevel - level) / counter;
					} else {
						double a1 = (endLevel - level) / (1.0 - exp(curve));	
						unit->m_a2 = level + a1;
						unit->m_b1 = a1; 
						unit->m_grow = exp(curve / counter);
					}
				} break;
				case shape_Squared : {
					unit->m_y1 = sqrt(level); 
					unit->m_y2 = sqrt(endLevel); 
					unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
				} break;
				case shape_Cubed : {
					unit->m_y1 = pow(level, 0.33333333); 
					unit->m_y2 = pow(endLevel, 0.33333333); 
					unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
				} break;
			}
		}
	}

	*out = level;
	switch (unit->m_shape) {
		case shape_Step : {
		} break;
		case shape_Linear : {
			double grow = unit->m_grow;
					//Print("level %g\n", level);
				level += grow;
		} break;
		case shape_Exponential : {
			double grow = unit->m_grow;
				level *= grow;
		} break;
		case shape_Sine : {
			double a2 = unit->m_a2;
			double b1 = unit->m_b1;
			double y2 = unit->m_y2;
			double y1 = unit->m_y1;
				double y0 = b1 * y1 - y2; 
				level = a2 - y0;
				y2 = y1; 
				y1 = y0;
			unit->m_y1 = y1;
			unit->m_y2 = y2;
		} break;
		case shape_Welch : {
			double a2 = unit->m_a2;
			double b1 = unit->m_b1;
			double y2 = unit->m_y2;
			double y1 = unit->m_y1;
				double y0 = b1 * y1 - y2; 
				level = a2 - y0;
				y2 = y1; 
				y1 = y0;
			unit->m_y1 = y1;
			unit->m_y2 = y2;
		} break;
		case shape_Curve : {
			double a2 = unit->m_a2;
			double b1 = unit->m_b1;
			double grow = unit->m_grow;
				b1 *= grow;
				level = a2 - b1;
			unit->m_b1 = b1;
		} break;
		case shape_Squared : {
			double grow = unit->m_grow;
			double y1 = unit->m_y1;
				y1 += grow;
				level = y1*y1;
			unit->m_y1 = y1;
		} break;
		case shape_Cubed : {
			double grow = unit->m_grow;
			double y1 = unit->m_y1;
				y1 += grow;
				level = y1*y1*y1;
			unit->m_y1 = y1;
		} break;
		case shape_Sustain : {
			if (gate <= 0.) {
				//Print("gate off %g\n", level);
				unit->m_released = true;
				counter = 1;
				break;
			}
		} break;
	}
	//Print("x %d %d %d %g\n", unit->m_stage, counter, unit->m_shape, *out);
	unit->m_level = level;
	unit->m_counter = counter - 1;

}

void BufEnvGen_next_ak(BufEnvGen *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float gate = ZIN0(1);
	int counter = unit->m_counter;
	double level = unit->m_level;
	
	if (unit->m_prevGate <= 0. && gate > 0.) {
		unit->m_stage = -1;
		unit->mDone = false;
		unit->m_released = false;
		counter = 0;
	}
	unit->m_prevGate = gate;
	
	int remain = inNumSamples;
	while (remain)
	{
		if (counter == 0) {
			// get table
			float fbufnum = ZIN0(0);
			if (fbufnum != unit->m_fbufnum) {
				unit->m_fbufnum = fbufnum;
				uint32 bufnum = (int)fbufnum;
				World *world = unit->mWorld;
				if (bufnum >= world->mNumSndBufs) bufnum = 0;
				unit->m_buf = world->mSndBufs + bufnum;
			}
			SndBuf *buf = unit->m_buf;
			int tableSize = buf->samples;

			if (unit->m_stage+1 >= buf->data[1]) { // num stages
				counter = INT_MAX;
				unit->m_shape = 0;
				level = unit->m_endLevel;
				unit->mDone = true;
			} else if (unit->m_stage == buf->data[2] && !unit->m_released) { // sustain stage
				counter = INT_MAX;
				unit->m_shape = shape_Sustain;
				level = unit->m_endLevel;
			} else {
				unit->m_stage++;
				//out = unit->m_level;
				int stageOffset = (unit->m_stage << 2) + 4;
				if (stageOffset + 4 > tableSize) {
					// oops.
					Print("envelope went past end of buffer.\n");
					ClearUnitOutputs(unit, 1);
					NodeEnd(&unit->mParent->mNode);
					return;
				}
				float* table = buf->data + stageOffset;
				double endLevel = table[0] * ZIN0(2) + ZIN0(3); // scale levels
				double dur      = table[1] * ZIN0(4);
				unit->m_shape   = (int32)table[2];
				double curve    = table[3];
				unit->m_endLevel = endLevel;
						
				counter  = (int32)(dur * SAMPLERATE);
				counter  = sc_max(1, counter);

				if (counter == 1) unit->m_shape = 1; // shape_Linear
				switch (unit->m_shape) {
					case shape_Step : {
						level = endLevel;
					} break;
					case shape_Linear : {
						unit->m_grow = (endLevel - level) / counter;
					} break;
					case shape_Exponential : {
						unit->m_grow = pow(endLevel / level, 1.0 / counter);
					} break;
					case shape_Sine : {
						double w = pi / counter;
			
						unit->m_a2 = (endLevel + level) * 0.5;
						unit->m_b1 = 2. * cos(w);
						unit->m_y1 = (endLevel - level) * 0.5;
						unit->m_y2 = unit->m_y1 * sin(pi * 0.5 - w);
						level = unit->m_a2 - unit->m_y1;
					} break;
					case shape_Welch : {
						double w = (pi * 0.5) / counter;
						
						unit->m_b1 = 2. * cos(w);
						
						if (endLevel >= level) {
							unit->m_a2 = level;
							unit->m_y1 = 0.;
							unit->m_y2 = -sin(w) * (endLevel - level);
						} else {
							unit->m_a2 = endLevel;
							unit->m_y1 = level - endLevel;
							unit->m_y2 = cos(w) * (level - endLevel);
						}
						level = unit->m_a2 + unit->m_y1;
					} break;
					case shape_Curve : {
						if (fabs(curve) < 0.001) {
							unit->m_shape = 1; // shape_Linear
							unit->m_grow = (endLevel - level) / counter;
						} else {
							double a1 = (endLevel - level) / (1.0 - exp(curve));	
							unit->m_a2 = level + a1;
							unit->m_b1 = a1; 
							unit->m_grow = exp(curve / counter);
						}
					} break;
					case shape_Squared : {
						unit->m_y1 = sqrt(level); 
						unit->m_y2 = sqrt(endLevel); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
					case shape_Cubed : {
						unit->m_y1 = pow(level, 0.33333333); 
						unit->m_y2 = pow(endLevel, 0.33333333); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
				}
			}
		}

		int nsmps = sc_min(remain, counter);
		switch (unit->m_shape) {
			case shape_Step : {
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
				}
			} break;
			case shape_Linear : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					level += grow;
				}
			} break;
			case shape_Exponential : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					level *= grow;
				}
			} break;
			case shape_Sine : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Welch : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Curve : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					b1 *= grow;
					level = a2 - b1;
				}
				unit->m_b1 = b1;
			} break;
			case shape_Squared : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Cubed : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Sustain : {
				if (gate <= 0.) {
					unit->m_released = true;
					counter = 0;
					nsmps = 0;
					break;
				}
				for (int i=0; i<nsmps; ++i) {
					ZXP(out) = level;
				}
			} break;
		}
		remain -= nsmps;
		counter -= nsmps;
	}
	//Print("x %d %d %d %g\n", unit->m_stage, counter, unit->m_shape, ZOUT0(0));
	unit->m_level = level;
	unit->m_counter = counter;

}

#define CHECK_GATE \
	gate = ZXP(gatein); \
	if (prevGate <= 0.f && gate > 0.f) { \
		unit->m_stage = -1; \
		unit->m_released = false; \
		unit->mDone = false; \
		counter = 0; \
		nsmps = i; \
		break; \
	}
	

void BufEnvGen_next_aa(BufEnvGen *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *gatein = ZIN(1);
	int counter = unit->m_counter;
	double level = unit->m_level;
	float gate;
	float prevGate = unit->m_prevGate;
	int remain = inNumSamples;
	while (remain)
	{
		if (counter == 0) {
			// get table
			float fbufnum = ZIN0(0);
			if (fbufnum != unit->m_fbufnum) {
				unit->m_fbufnum = fbufnum;
				uint32 bufnum = (int)fbufnum;
				World *world = unit->mWorld;
				if (bufnum >= world->mNumSndBufs) bufnum = 0;
				unit->m_buf = world->mSndBufs + bufnum;
			}
			SndBuf *buf = unit->m_buf;
			int tableSize = buf->samples;

			if (unit->m_stage+1 >= buf->data[1]) { // num stages
				counter = INT_MAX;
				unit->m_shape = 0;
				level = unit->m_endLevel;
				unit->mDone = true;
			} else if (unit->m_stage == buf->data[2] && !unit->m_released) { // sustain stage
				counter = INT_MAX;
				unit->m_shape = shape_Sustain;
				level = unit->m_endLevel;
			} else {
				unit->m_stage++;
				//out = unit->m_level;
				int stageOffset = (unit->m_stage << 2) + 4;
				if (stageOffset + 4 > tableSize) {
					// oops.
					Print("envelope went past end of buffer.\n");
					ClearUnitOutputs(unit, remain);
					NodeEnd(&unit->mParent->mNode);
					return;
				}
				float* table = buf->data + stageOffset;
				double endLevel = table[0] * ZIN0(2) + ZIN0(3); // scale levels
				double dur      = table[1] * ZIN0(4);
				unit->m_shape   = (int32)table[2];
				double curve    = table[3];
				unit->m_endLevel = endLevel;
						
				counter  = (int32)(dur * SAMPLERATE);
				counter  = sc_max(1, counter);

				if (counter == 1) unit->m_shape = 1; // shape_Linear
				switch (unit->m_shape) {
					case shape_Step : {
						level = endLevel;
					} break;
					case shape_Linear : {
						unit->m_grow = (endLevel - level) / counter;
					} break;
					case shape_Exponential : {
						unit->m_grow = pow(endLevel / level, 1.0 / counter);
					} break;
					case shape_Sine : {
						double w = pi / counter;
			
						unit->m_a2 = (endLevel + level) * 0.5;
						unit->m_b1 = 2. * cos(w);
						unit->m_y1 = (endLevel - level) * 0.5;
						unit->m_y2 = unit->m_y1 * sin(pi * 0.5 - w);
						level = unit->m_a2 - unit->m_y1;
					} break;
					case shape_Welch : {
						double w = (pi * 0.5) / counter;
						
						unit->m_b1 = 2. * cos(w);
						
						if (endLevel >= level) {
							unit->m_a2 = level;
							unit->m_y1 = 0.;
							unit->m_y2 = -sin(w) * (endLevel - level);
						} else {
							unit->m_a2 = endLevel;
							unit->m_y1 = level - endLevel;
							unit->m_y2 = cos(w) * (level - endLevel);
						}
						level = unit->m_a2 + unit->m_y1;
					} break;
					case shape_Curve : {
						if (fabs(curve) < 0.001) {
							unit->m_shape = 1; // shape_Linear
							unit->m_grow = (endLevel - level) / counter;
						} else {
							double a1 = (endLevel - level) / (1.0 - exp(curve));	
							unit->m_a2 = level + a1;
							unit->m_b1 = a1; 
							unit->m_grow = exp(curve / counter);
						}
					} break;
					case shape_Squared : {
						unit->m_y1 = sqrt(level); 
						unit->m_y2 = sqrt(endLevel); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
					case shape_Cubed : {
						unit->m_y1 = pow(level, 0.33333333); 
						unit->m_y2 = pow(endLevel, 0.33333333); 
						unit->m_grow = (unit->m_y2 - unit->m_y1) / counter;
					} break;
				}
			}
		}

		int nsmps = sc_min(remain, counter);
		switch (unit->m_shape) {
			case shape_Step : {
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
				}
			} break;
			case shape_Linear : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					level += grow;
				}
			} break;
			case shape_Exponential : {
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					level *= grow;
				}
			} break;
			case shape_Sine : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Welch : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double y2 = unit->m_y2;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
				unit->m_y1 = y1;
				unit->m_y2 = y2;
			} break;
			case shape_Curve : {
				double a2 = unit->m_a2;
				double b1 = unit->m_b1;
				double grow = unit->m_grow;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					b1 *= grow;
					level = a2 - b1;
				}
				unit->m_b1 = b1;
			} break;
			case shape_Squared : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Cubed : {
				double grow = unit->m_grow;
				double y1 = unit->m_y1;
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					ZXP(out) = level;
					y1 += grow;
					level = y1*y1*y1;
				}
				unit->m_y1 = y1;
			} break;
			case shape_Sustain : {
				for (int i=0; i<nsmps; ++i) {
					CHECK_GATE
					if (gate <= 0.) {
						unit->m_released = true;
						counter = 0;
						nsmps = i;
						break;
					}
					ZXP(out) = level;
				}
			} break;
		}
		remain -= nsmps;
		counter -= nsmps;
	}
	unit->m_level = level;
	unit->m_counter = counter;
	unit->m_prevGate = gate;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

void Linen_Ctor(Linen *unit)
{
	// gate attack level release
	SETCALC(Linen_next_k);
	
	unit->m_level = 0.f;
	unit->m_stage = 4;
	unit->m_prevGate = 0.f;
	Linen_next_k(unit, 1);
}

void Linen_next_k(Linen *unit, int inNumSamples)
{
	float gate = ZIN0(0);
	float *out = OUT(0);
		
	if (unit->m_prevGate <= 0.f && gate > 0.f) {
		unit->mDone = false;
		unit->m_stage = 0;
		float attackTime = ZIN0(1);
		float susLevel = ZIN0(2);
		int counter = (int)(attackTime * SAMPLERATE);
		counter = sc_max(1, counter);
		unit->m_slope = (susLevel - unit->m_level) / counter;
		unit->m_counter = counter;
	}
	switch (unit->m_stage) {
		case 0 : 
		case 2 : 
			*out = unit->m_level;
			unit->m_level += unit->m_slope;
			if (--unit->m_counter == 0) unit->m_stage++;
			break;
		case 1 : 
			*out = unit->m_level;
			if (gate <= 0.f) {
				unit->m_stage = 2;
				float releaseTime = ZIN0(3);
				int counter = (int)(releaseTime * SAMPLERATE);
				counter = sc_max(1, counter);
				unit->m_slope = -unit->m_level / counter;
				unit->m_counter = counter;
				
				//Print("release %d %d\n", unit->mParent->mNode.mID, counter);
			}
			break;
		case 3 : {
			*out = 0.f;			
			//Print("done %d\n", unit->mParent->mNode.mID);
			unit->mDone = true;
			unit->m_stage++;
			int doneAction = (int)ZIN0(4);
			DoneAction(doneAction, unit);
		} break;
		case 4 :
			*out = 0.f;
			break;
	}
	unit->m_prevGate = gate;
}


#if 0

#define LINEN_GATE \
	if (unit->m_prevGate <= 0.f && gate > 0.f) { \
		unit->m_stage = 0; \
		float attackTime = ZIN0(1); \
		float susLevel = ZIN0(2); \
		int counter = (int)(attackTime * SAMPLERATE); \
		counter = sc_max(1, counter); \
		slope = (susLevel - level) / counter; \
	}

void Linen_next_ak(Linen *unit, int inNumSamples)
{
	float *gate = ZIN0(0);
	float *out = ZOUT(0);
	
	int stage = unit->m_stage;
	int counter = unit->m_counter;
	double level = unit->m_level;
	double slope = unit->m_slope;
	
	if (unit->m_prevGate <= 0.f && gate > 0.f) {
		unit->m_stage = 0;
		float attackTime = ZIN0(1);
		float susLevel = ZIN0(2);
		int counter = attackTime * SAMPLERATE;
		counter = sc_max(1, counter);
		slope = (susLevel - level) / counter;
	}

	int remain = inNumSamples;
	do {
		int nsmps = sc_min(remain, counter);
		switch (stage) {
			case 0 : 
				LOOP(nsmps, 
					ZXP(out) = level;
					level += slope;
				);
				counter -= nsmps;
				if (counter == 0) {
					stage = 1;
					counter = INT_MAX;
				}
				break;
			case 1 : 
				LOOP(nsmps, 
					ZXP(out) = level;
				);
				if (gate <= 0.f) {
					unit->m_stage = 2;
					float releaseTime = ZIN0(3);
					int counter = releaseTime * SAMPLERATE;
					counter = sc_max(1, counter);
					unit->m_slope = -level / counter;
					counter = counter;
				}
				break;
			case 2 : 
				LOOP(nsmps, 
					ZXP(out) = level;
					level += slope;
				);
				counter -= nsmps;
				if (counter == 0) {
					stage = 3;
					counter = INT_MAX;
				}
				break;
			case 3 :
				*out = 0.f;
				break;
		}
	} while (remain);
	
	unit->m_stage = stage;
	unit->m_prevGate = gate;
	unit->m_counter = counter;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

void ADSR_Ctor(ADSR *unit)
{
	// gate attack level release
	SETCALC(ADSR_next_k);
	
	unit->m_level = 0.f;
	unit->m_stage = 5;
	unit->m_prevGate = 0.f;
	ADSR_next_k(unit, 1);
}

inline void ADSR_set(ADSR *unit, float endLevel, float time)
{
	float curve = ZIN0(6);
	int counter = (int)(time * SAMPLERATE);
	counter = sc_max(1, counter);
	unit->m_counter = counter;

	double a1 = (endLevel - unit->m_level) / (1.0 - exp(curve));	
	unit->m_a2 = unit->m_level + a1;
	unit->m_b1 = a1; 
	unit->m_grow = exp(curve / counter);
}

inline void ADSR_next(ADSR *unit)
{
	unit->m_b1 *= unit->m_grow;
	unit->m_level = unit->m_a2 - unit->m_b1;
}

void ADSR_next_k(ADSR *unit, int inNumSamples)
{
	// gate a, peak level, d s r curve
	float gate = ZIN0(0); 
	float *out = OUT(0);
		
	if (unit->m_prevGate <= 0.f && gate > 0.f) {
		unit->mDone = false;
		unit->m_stage = 0;
		float attackTime = ZIN0(1);
		float peakLevel = ZIN0(2);
		ADSR_set(unit, peakLevel, attackTime);
	}
	switch (unit->m_stage) {
		case 0 : // attack
			*out = unit->m_level;
			ADSR_next(unit);
			if (--unit->m_counter == 0) {
				unit->m_stage++;
				float decayTime = ZIN0(3);
				float susLevel = ZIN0(4);
				ADSR_set(unit, susLevel, decayTime);
			}
			break;
		case 1 : // decay
		case 3 : // release
			*out = unit->m_level;
			ADSR_next(unit);
			if (--unit->m_counter == 0) {
				unit->m_stage++;
			}
			break;
		case 2 : // sustain -> release
			*out = unit->m_level;
			if (gate <= 0.f) {
				unit->m_stage++;
				float releaseTime = ZIN0(5);
				ADSR_set(unit, 0.f, releaseTime);
			}
			break;
		case 4 : { // done
				unit->mDone = true;
				*out = 0.f;
				unit->m_stage++;
				int doneAction = (int)ZIN0(6);
				DoneAction(doneAction, unit);
			} break;
		case 5 : // done
			*out = 0.f;
			break;
	}
	unit->m_prevGate = gate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////


void EnvFill(World *world, struct SndBuf *buf, struct sc_msg_iter *msg)
{
	if (buf->channels != 1) return;
		
	int size = buf->samples;
	int byteSize = size * sizeof(float);
	float *data = (float*)malloc(byteSize);

	double level = msg->getf();
	int numStages = msg->geti();
	/*int releaseNode =*/ msg->geti(); // ignored
	/*int loopNode =*/ msg->geti(); // ignored
	
	double pos = 0.;
	int32 index = 0;
	int32 remain = size;
	
	for (int j=0; j < numStages; ++j)
	{
		double endLevel = msg->getf();
		double dur = msg->getf();
		int shape = msg->geti(); 
		double curve = msg->getf();

		int32 ipos = (int32)pos;
		double smpdur = dur * size;
		int32 nsmps = (int32)smpdur - ipos;
		nsmps = sc_min(nsmps, remain);

		switch (shape) {
			case shape_Step : {
				level = endLevel;
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
				}
			} break;
			case shape_Linear : {
				double grow = (endLevel - level) / nsmps;
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
					level += grow;
				}
			} break;
			case shape_Exponential : {
				double grow = pow(endLevel / level, 1.0 / nsmps);
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
					level *= grow;
				}
			} break;
			case shape_Sine : {
				double w = pi / nsmps;
	
				double a2 = (endLevel + level) * 0.5;
				double b1 = 2. * cos(w);
				double y1 = (endLevel - level) * 0.5;
				double y2 = y1 * sin(pi * 0.5 - w);
				level = a2 - y1;
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
			} break;
			case shape_Welch : {
				double w = (pi * 0.5) / nsmps;
				
				double b1 = 2. * cos(w);
				double a2, y1, y2;
				if (endLevel >= level) {
					a2 = level;
					y1 = 0.;
					y2 = -sin(w) * (endLevel - level);
				} else {
					a2 = endLevel;
					y1 = level - endLevel;
					y2 = cos(w) * (level - endLevel);
				}
				level = a2 + y1;
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
					double y0 = b1 * y1 - y2; 
					level = a2 - y0;
					y2 = y1; 
					y1 = y0;
				}
			} break;
			case shape_Curve : {
				if (fabs(curve) < 0.001) {
					double grow = (endLevel - level) / nsmps;
					for (int i=0; i<nsmps; ++i) {
						data[index++] = level;
						level += grow;
					}
				} else {
					double a1 = (endLevel - level) / (1.0 - exp(curve));	
					double a2 = level + a1;
					double b1 = a1; 
					double grow = exp(curve / nsmps);
					for (int i=0; i<nsmps; ++i) {
						data[index++] = level;
						b1 *= grow;
						level = a2 - b1;
					}
				}
			} break;
			case shape_Squared : {
				double y1 = sqrt(level); 
				double y2 = sqrt(endLevel); 
				double grow = (y2 - y1) / nsmps;
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
					y1 += grow;
					level = y1*y1;
				}
			} break;
			case shape_Cubed : {
				double y1 = pow(level, 0.33333333); 
				double y2 = pow(endLevel, 0.33333333); 
				double grow = (y2 - y1) / nsmps;
				for (int i=0; i<nsmps; ++i) {
					data[index++] = level;
					y1 += grow;
					level = y1*y1*y1;
				}
			} break;
		}

		pos += smpdur;
		level = endLevel;
		remain -= nsmps;
	}
	memcpy(buf->data, data, byteSize);
	free(data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////


void load(InterfaceTable *inTable)
{
	ft = inTable;

	DefineSimpleUnit(Vibrato);
	DefineSimpleUnit(LFPulse);
	DefineSimpleUnit(LFSaw);
	DefineSimpleUnit(LFPar);
	DefineSimpleUnit(LFCub);
	DefineSimpleUnit(LFTri);
	DefineSimpleUnit(Impulse);
	DefineSimpleUnit(VarSaw);
	DefineSimpleUnit(SyncSaw);
	DefineSimpleUnit(K2A);
	DefineSimpleUnit(Silent);
	DefineSimpleUnit(Line);
	DefineSimpleUnit(XLine);
	
	DefineSimpleUnit(Wrap);
	DefineSimpleUnit(Fold);
	DefineSimpleUnit(Clip);
	DefineSimpleUnit(Unwrap);
	DefineSimpleUnit(InRange);
	DefineSimpleUnit(InRect);
	DefineSimpleUnit(LinExp);
	DefineSimpleUnit(LinLin);
	DefineSimpleUnit(EnvGen);
	//DefineSimpleUnit(BufEnvGen);
	DefineSimpleUnit(Linen);

	DefineBufGen("env", EnvFill);
}
