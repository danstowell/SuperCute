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

//Partitioned Convolution, Nick Collins mid October 2008
//PartConv(in, fftsize, irbufnum, accumbufnum)


#include "FFT_UGens.h"


struct PartConv : public Unit {
	int m_counter;
	uint32 m_specbufnumcheck;
	float* m_fd_accumulate; //will be exactly fftsize*frames in size
	float * m_irspectra;
	int m_fd_accum_pos;
	int m_partitions; //number of frames impulse response is partitioned into
	int m_fullsize;

	//sliding window code
	int m_fftsize; //must be power of two
	//int m_windowsize; //also half fftsize, was partition size, just use nover2 for this
	int m_nover2;

	//int m_hopsize; //hopsize will be half fftsize
	//int m_shuntsize;
	int m_pos;
	float * m_inputbuf;
	float * m_spectrum;
	scfft* m_scfft;
	float * m_inputbuf2;
	float * m_spectrum2;
	scfft* m_scifft; //inverse
	float * m_transformbuf;
	int m_outputpos;
	float * m_output;

	//amortisation state
	int m_blocksize, m_sr;
	int m_spareblocks;
	int m_numamort; //will relate number of partitions to number of spare blocks
	int m_lastamort;
	int m_amortcount;
	int m_partitionsdone;
};


extern "C" {

	void PartConv_next(PartConv *unit, int inNumSamples);
	void PartConv_Ctor(PartConv* unit);
	void PartConv_Dtor(PartConv* unit);

}

void PartConv_Ctor( PartConv* unit ) {

	unit->m_fftsize= (int) ZIN0(1);
	unit->m_nover2= unit->m_fftsize>>1;
	//unit->m_windowsize= unit->m_fftsize>>1;

	//unit->m_hopsize= unit->m_fftsize>>1;
	//unit->m_shuntsize== unit->hopsize;



	//integer division

	unit->m_inputbuf= (float*)RTAlloc(unit->mWorld, unit->m_fftsize * sizeof(float));
	unit->m_spectrum= (float*)RTAlloc(unit->mWorld, unit->m_fftsize * sizeof(float));
	unit->m_transformbuf = (float*)RTAlloc(unit->mWorld, scfft_trbufsize(unit->m_fftsize));
	unit->m_scfft = (scfft*)RTAlloc(unit->mWorld, sizeof(scfft));
	scfft_create(unit->m_scfft, unit->m_fftsize, unit->m_fftsize, WINDOW_RECT, unit->m_inputbuf, unit->m_spectrum, unit->m_transformbuf, true);

	//inverse
	unit->m_inputbuf2= (float*)RTAlloc(unit->mWorld, unit->m_fftsize * sizeof(float));
	unit->m_spectrum2= (float*)RTAlloc(unit->mWorld, unit->m_fftsize * sizeof(float));
	unit->m_scifft = (scfft*)RTAlloc(unit->mWorld, sizeof(scfft));
	//in place this time
	scfft_create(unit->m_scifft, unit->m_fftsize, unit->m_fftsize, WINDOW_RECT, unit->m_inputbuf2, unit->m_spectrum2, unit->m_transformbuf, false);

	//debug test: changing scale factors in case amplitude summation is a problem
	//unit->m_scfft->scalefac=1.0/45.254833995939;
	//unit->m_scifft->scalefac=1.0/45.254833995939;



	unit->m_output= (float*)RTAlloc(unit->mWorld, unit->m_fftsize * sizeof(float));
	unit->m_outputpos=0;

	memset(unit->m_output, 0, unit->m_fftsize * sizeof(float));

	memset(unit->m_inputbuf, 0, unit->m_fftsize * sizeof(float));
	unit->m_pos=0;

	//get passed in buffer
	unit->m_fd_accumulate=NULL;

	uint32 bufnum = (uint32)ZIN0(2);
	SndBuf *buf;

	if (bufnum >=  unit->mWorld->mNumSndBufs) {

		int localBufNum = bufnum - unit->mWorld->mNumSndBufs;
		Graph *parent = unit->mParent;
		if(localBufNum <= parent->localMaxBufNum) {
			buf = parent->mLocalSndBufs + localBufNum;

		} else {

			printf("PartConv Error: Invalid Spectral data bufnum %d \n", bufnum);
		SETCALC(*ClearUnitOutputs);
		unit->mDone = true;
		return;

		}
	}

	buf = unit->mWorld->mSndBufs + bufnum;

	unit->m_specbufnumcheck = bufnum;

	//buf = unit->mWorld->mSndBufs + bufnum;


	if (!buf->data) {
		//unit->mDone = true;
		printf("PartConv Error: Spectral data buffer not allocated \n");
		SETCALC(*ClearUnitOutputs);
		unit->mDone = true;
		return;
	}

	unit->m_irspectra = buf->data;

	unit->m_fullsize = buf->samples;

	unit->m_partitions=buf->samples/(unit->m_fftsize); //should be exact
	//printf("another check partitions %d irspecsize %d fftsize %d \n", unit->m_partitions,unit->m_fullsize, unit->m_fftsize);

	if((buf->samples % unit->m_fftsize !=0) || (buf->samples==0)) {
		printf("PartConv Error: fftsize doesn't divide spectral data buffer size or spectral data buffer size is zero\n");
		SETCALC(*ClearUnitOutputs);
		unit->mDone = true;
		return;
	}

	//CHECK SAMPLING RATE AND BUFFER SIZE
	unit->m_blocksize = unit->mWorld->mFullRate.mBufLength;
	//if(unit->m_blocksize!=64) printf("TPV complains: block size not 64, you have %d\n", unit->m_blocksize);
	unit->m_sr = unit->mWorld->mSampleRate;
	//if(unit->m_sr!=44100) printf("TPV complains: sample rate not 44100, you have %d\n", unit->m_sr);

	if(unit->m_nover2 % unit->m_blocksize !=0) {
		printf("PartConv Error: block size doesn't divide partition size\n");
		SETCALC(*ClearUnitOutputs);
		unit->mDone = true;
		return;
	}
	else {

	//must be exact divisor
	int blocksperpartition = unit->m_nover2/unit->m_blocksize;

	unit->m_spareblocks = blocksperpartition-1;

	if(unit->m_spareblocks<1) {
	printf("PartConv Error: no spareblocks, amortisation not possible! \n");
	SETCALC(*ClearUnitOutputs);
	unit->mDone = true;
	return;
	}

	//won't be exact
	unit->m_numamort = (unit->m_partitions-1)/unit->m_spareblocks; //will relate number of partitions to number of spare blocks
	unit->m_lastamort= (unit->m_partitions-1)- ((unit->m_spareblocks-1)*(unit->m_numamort)); //allow for error on last one
	unit->m_amortcount= -1; //starts as flag to avoid any amortisation before have first fft done
	unit->m_partitionsdone=1;

	//printf("Amortisation stats partitions %d nover2 %d blocksize %d spareblocks %d numamort %d lastamort %d \n", unit->m_partitions,unit->m_nover2, unit->m_blocksize, unit->m_spareblocks, unit->m_numamort, unit->m_lastamort);

	unit->m_fd_accumulate= (float*)RTAlloc(unit->mWorld, unit->m_fullsize * sizeof(float));
	memset(unit->m_fd_accumulate, 0, unit->m_fullsize * sizeof(float));
	unit->m_fd_accum_pos=0;

//	used to be passed in as buffer, simplified interface
//	bufnum = (uint32)ZIN0(3);
//	if (bufnum >= unit->mWorld->mNumSndBufs) {
//		bufnum = 1;
//	}
//	buf =  unit->mWorld->mSndBufs + bufnum;
//
//	if (!buf->data) {
//		printf("PartConv Error: Accumulation buffer not allocated \n");
//		SETCALC(*ClearUnitOutputs);
//		unit->mDone = true;
//		return;
//	}
//
//	unit->m_fd_accumulate = buf->data;


	SETCALC(PartConv_next);
	}

}

void PartConv_Dtor(PartConv *unit) {
	RTFree(unit->mWorld, unit->m_inputbuf);
	RTFree(unit->mWorld, unit->m_inputbuf2);
	RTFree(unit->mWorld, unit->m_spectrum);
	RTFree(unit->mWorld, unit->m_spectrum2);
	RTFree(unit->mWorld, unit->m_transformbuf);
	RTFree(unit->mWorld, unit->m_output);
	if (unit->m_fd_accumulate) RTFree(unit->mWorld, unit->m_fd_accumulate);

	if(unit->m_scfft){
		scfft_destroy(unit->m_scfft);
		RTFree(unit->mWorld, unit->m_scfft);
	}

	if(unit->m_scifft){
		scfft_destroy(unit->m_scifft);
		RTFree(unit->mWorld, unit->m_scifft);
	}
}

void PartConv_next( PartConv *unit, int inNumSamples ) {

	int i,j;
	float *in = IN(0);
	float *out = OUT(0);
	int pos = unit->m_pos;

	//safety check
	if (!(unit->mWorld->mSndBufs + unit->m_specbufnumcheck)->data) {
		//unit->mDone = true;
		printf("PartConv Error: Spectral data buffer not allocated \n");
		ClearUnitOutputs(unit, inNumSamples);
		SETCALC(*ClearUnitOutputs);
		unit->mDone = true;
		return;
	}


	float * input= unit->m_inputbuf;
	float * output= unit->m_output;
	int outputpos= unit->m_outputpos;

	//into input buffer
	memcpy(input+pos, in, inNumSamples * sizeof(float));

	pos+=inNumSamples;

	//if ready for new FFT
	int nover2= unit->m_nover2;

	//assumes that blocksize perfectly divides windowsize
	if (pos==nover2) {
		//FFT this input, second half of input always zero

		//memset(input+unit->m_nover2, 0, sizeof(float)*unit->m_nover2);

		scfft_dofft(unit->m_scfft);

		//reset pos into input buffer
		pos=0;
		//reset outputpos
		outputpos= 0;

		float * spectrum= unit->m_spectrum;
		float * spectrum2= unit->m_spectrum2;

		//multiply spectra and accumulate for all ir spectra across storage buffer

		int fftsize= unit->m_fftsize;
		//int frames= unit->m_partitions;
		int accumpos= unit->m_fd_accum_pos;
		float * accumbuffer= unit->m_fd_accumulate;

		float * irspectrum= unit->m_irspectra;

		int fullsize= unit->m_fullsize;

		//JUST DO FIRST ONE FOR NOW, AMORTISED FOR OTHERS
		//frames
		for (i=0; i<1; ++i) {

			int irpos= (i*fftsize);
			int posnow= (accumpos+irpos)%fullsize;
			float * target= accumbuffer+posnow;
			float * ir= irspectrum+irpos;

			//real multiply for dc and nyquist
			target[0] += ir[0]*spectrum[0];
			target[1] += ir[1]*spectrum[1];

			//complex multiply for frequency bins
			for (j=1; j<nover2; ++j) {

				int binposr= 2*j;
				int binposi= binposr+1;

				//complex multiply
				target[binposr] += (ir[binposr]*spectrum[binposr]) - (ir[binposi]*spectrum[binposi]);
				target[binposi] += (ir[binposi]*spectrum[binposr]) + (ir[binposr]*spectrum[binposi]);
			}


	//LESS efficient!
		//	float * irreal = ir;
//			float * irimag = ir+1;
//			float * specreal = spectrum;
//			float * specimag = spectrum+1;
//			float * targetreal= target;
//			float * targetimag= target+1;
//
//			for (j=1; j<nover2; ++j) {
//
//				irreal+=2; irimag+=2;
//				specreal+=2; specimag+=2;
//				targetreal+=2; targetimag+=2;
//
//				//complex multiply
//				(*targetreal)+=  (*irreal)*(*specreal) - (*irimag)*(*specimag);
//				(*targetimag)+=  (*irimag)*(*specreal) - (*irreal)*(*specimag);
//			}



		}

		//need separate scfft object with forward= false
		//IFFT this partition
		float * input2= unit->m_inputbuf2;
		memcpy(input2, accumbuffer+accumpos, fftsize * sizeof(float));
		scfft_doifft(unit->m_scifft);

		//shunt output data down and zero top bit
		memcpy(output, output+nover2, nover2 * sizeof(float));
		memset(output+nover2, 0, nover2 * sizeof(float));

		//surely last entry should be zero?
		//printf("sanity check %f \n",spectrum2[0]); //spectrum2[fftsize-1]
		//(fftsize-1)

		//sum into output
		for (j=0; j<fftsize; ++j)
			output[j] += spectrum2[j];

		//int testindex2= rgen.irand(fftsize-1);
		//printf("outputtest j %d value %f index %d value2 %f \n",testindex2 , output[testindex2], testindex2, *(accumbuffer+accumpos+testindex2));

		//zero this partition
		memset(accumbuffer+accumpos, 0, fftsize * sizeof(float));

		//ONLY DO AT END OF AMORTISATION???? no, amort code has extra -1 in indexing to cope
		//update partition counter
		accumpos= (accumpos+fftsize)%fullsize;
		unit->m_fd_accum_pos= accumpos;

		//set up for amortisation (calculate output for other partitions of impulse response)
		unit->m_amortcount=0;
		unit->m_partitionsdone=1;
	}
	else {
	//amortisation steps:
	//complex multiply of this new fft spectrum against existing irspectrums and sum to accumbuffer
		if (unit->m_amortcount>=0) {

		float * spectrum= unit->m_spectrum;

		//multiply spectra and accumulate for all ir spectra across storage buffer

		int fftsize= unit->m_fftsize;
		int nover2= unit->m_nover2;
		//int frames= unit->m_partitions;
		int accumpos= unit->m_fd_accum_pos;
		float * accumbuffer= unit->m_fd_accumulate;

		float * irspectrum= unit->m_irspectra;

		int fullsize= unit->m_fullsize;

		int starti, stopi;
		int number;

		if(unit->m_amortcount==(unit->m_spareblocks-1)) {
			number= unit->m_lastamort;
		}
		else
		{
			number= unit->m_numamort;
		}

		starti= unit->m_partitionsdone-1;
		stopi= starti+number-1;

		//printf("amort check count %d starti %d stopi %d number %d framesdone %d \n",unit->m_amortcount, starti, stopi, number, unit->m_partitionsdone);

		unit->m_partitionsdone += number;
		++unit->m_amortcount;


		//JUST DO FIRST ONE FOR NOW
		//frames
		for (i=starti; i<=stopi; ++i) {

			int posnow= (accumpos+(i*fftsize))%fullsize;
			float * target= accumbuffer+posnow;
			int irpos= (i*fftsize);
			float * ir= irspectrum+irpos;

			//real multiply for dc and nyquist
			target[0]+=  ir[0]*spectrum[0];
			target[1]+=	 ir[1]*spectrum[1];

			//complex multiply for frequency bins
			for (j=1; j<nover2; ++j) {

				int binposr= 2*j;
				int binposi= binposr+1;

				//complex multiply
				target[binposr]+=  (ir[binposr]*spectrum[binposr]) - (ir[binposi]*spectrum[binposi]);
				target[binposi]+=  (ir[binposi]*spectrum[binposr]) + (ir[binposr]*spectrum[binposi]);

			}

			//printf("posnow %d i %d dc %f nyquist %f\n", posnow, i, target[400], target[405]);

		}

		//if(unit->m_amortcount==(unit->m_spareblocks-1)) {
		//	number= unit->m_numamort;
		//}

		}

	}


	//control rate, just one out value
	//output max
	//for(j=0; j<inNumSamples; ++j) {
	//out[j]= outputbuf[outputpos+j];
	//}


	//do this second!
	memcpy(out, output+outputpos, inNumSamples * sizeof(float));

//debugging tests: output values tend to be fftsize too big, probably due to complex multiply and also summation over all partitions
//	RGen& rgen = *unit->mParent->mRGen;
//	int testindex= rgen.irand(inNumSamples-1);
//	printf("inNumSamples %d testindex %d out %f output %f \n",inNumSamples, testindex, out[testindex], *(output+outputpos+testindex));
//
	outputpos+=inNumSamples;

	unit->m_outputpos= outputpos;

	unit->m_pos= pos;


}





//channels not used- should just be mono, num frames= num samples
//buffer preparation
void PreparePartConv(World *world, struct SndBuf *buf, struct sc_msg_iter *msg)
{
	int i,j;

	int frames1 = buf->frames;
	//int channels1 = buf->channels;
	float *data1 = buf->data;

	uint32 frombufnum = msg->geti();
	int fftsize = msg->geti();

	//output size must be frombuf->frames*2

	if (frombufnum >= world->mNumSndBufs) frombufnum = 0;
	SndBuf* frombuf = world->mSndBufs + frombufnum;

	int frames2 = frombuf->frames;
	//int channels2 = frombuf->channels;

	float *data2 = frombuf->data;
	//int size = buf->samples;

	//scfft
	int nover2= fftsize>>1;

	//int numpartitions = msg->geti();

	int numpartitions;
	if(frames2%nover2==0)
		numpartitions= frames2/nover2;
	else
		numpartitions= (frames2/nover2)+1;

	//check numpartitions*fftsize= frames2

	//printf("reality check numpartitions %d fftsize %d product %d numirframes %d numinputframes %d \n", numpartitions, fftsize, numpartitions*fftsize, frames1, frames2);

	//integer division

	float * inputbuf= (float*)RTAlloc(world, fftsize * sizeof(float));
	float * spectrum= (float*)RTAlloc(world, fftsize * sizeof(float));
	float * transformbuf = (float*)RTAlloc(world, scfft_trbufsize(fftsize));
	scfft* m_scfft = (scfft*)RTAlloc(world, sizeof(scfft));
	scfft_create(m_scfft, fftsize, fftsize, -1, inputbuf,spectrum,transformbuf, true);

	//for zero padding
	memset(inputbuf, 0, sizeof(float)*fftsize);

	//run through input data buffer, taking nover2 chunks, zero padding each
	for (i=0; i<numpartitions; ++i) {
		int indexnow= nover2*i;
		int indexout= fftsize*i;

		if(i<(numpartitions-1))
		//memset(inputbuf, 0, sizeof(float)*fftsize);
		memcpy(inputbuf, data2+indexnow, nover2 * sizeof(float));
		else
		{
		int takenow= frames2%nover2;

		if(frames2==nover2)
		takenow= nover2;

		memcpy(inputbuf, data2+indexnow, takenow * sizeof(float));

		if(takenow<nover2)
		memset(inputbuf+takenow, 0, (nover2-takenow)*sizeof(float));

		}

		scfft_dofft(m_scfft);

		memcpy(data1+indexout, spectrum, fftsize * sizeof(float));

	}

	//clean up
	RTFree(world, inputbuf);
	RTFree(world, spectrum);
	RTFree(world, transformbuf);

	if(m_scfft){
		scfft_destroy(m_scfft);
		RTFree(world, m_scfft);
	}


}



void initPartConv(InterfaceTable *inTable) {

	ft = inTable;

	DefineDtorCantAliasUnit(PartConv);

	DefineBufGen("PreparePartConv", PreparePartConv);
}


