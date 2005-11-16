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

#include "SC_CoreAudio.h"
#include "SC_Sem.h"
#include "SC_ComPort.h"
#include <stdarg.h>
#include "SC_SequencedCommand.h"
#include "SC_Prototypes.h"
#include "SC_HiddenWorld.h"
#include "SC_Endian.h"
#include "SC_Lib_Cintf.h"
#include <stdlib.h>
#include <pthread.h>

#ifdef SC_WIN32
#include "SC_Win32Utils.h"
#endif

#ifndef SC_INNERSC
int64 gStartupOSCTime = -1;
#endif //ifndef SC_INNERSC

void set_real_time_priority(pthread_t thread);

double gSampleRate, gSampleDur;

// =====================================================================
// Timing (CoreAudio)

#if SC_AUDIO_API == SC_AUDIO_API_COREAUDIO

int64 gOSCoffset = 0; 

int32 server_timeseed()
{
	static int32 count = 0;
	int64 time = AudioGetCurrentHostTime();
	return (int32)(time >> 32) ^ (int32)time ^ count--;
}

inline int64 CoreAudioHostTimeToOSC(int64 hostTime)
{
	return (int64)((double)AudioConvertHostTimeToNanos(hostTime) * kNanosToOSCunits) + gOSCoffset;
}

int64 oscTimeNow()
{
	return CoreAudioHostTimeToOSC(AudioGetCurrentHostTime());
}

void syncOSCOffsetWithTimeOfDay();
void syncOSCOffsetWithTimeOfDay()
{
	// generate a value gOSCoffset such that  
	// (gOSCOffset + systemTimeInOSCunits) 
	// is equal to gettimeofday time in OSCunits.
	// Then if this machine is synced via NTP, we are synced with the world.
	// more accurate way to do this??

	struct timeval tv;	
	
	int64 systemTimeBefore, systemTimeAfter, diff;
	int64 minDiff = 0x7fffFFFFffffFFFFLL;
	
	// take best of several tries
	const int numberOfTries = 5;
	int64 newOffset = gOSCoffset;
	for (int i=0; i<numberOfTries; ++i) {
		systemTimeBefore = AudioGetCurrentHostTime();
		gettimeofday(&tv, 0);
		systemTimeAfter = AudioGetCurrentHostTime();
		
		diff = systemTimeAfter - systemTimeBefore;
		if (diff < minDiff) {
			minDiff = diff;
			// assume that gettimeofday happens halfway between AudioGetCurrentHostTime calls
			int64 systemTimeBetween = systemTimeBefore + diff/2;
			int64 systemTimeInOSCunits = (int64)((double)AudioConvertHostTimeToNanos(systemTimeBetween) * kNanosToOSCunits); 
			int64 timeOfDayInOSCunits  = ((int64)(tv.tv_sec + kSECONDS_FROM_1900_to_1970) << 32) 
								    + (int64)(tv.tv_usec * kMicrosToOSCunits);
			newOffset = timeOfDayInOSCunits - systemTimeInOSCunits;
		}
	}
	
	gOSCoffset = newOffset;
	//scprintf("gOSCoffset %016llX\n", gOSCoffset);
}

void* resyncThreadFunc(void* arg);
void* resyncThreadFunc(void* /*arg*/)
{
	while (true) {
		sleep(20);
		syncOSCOffsetWithTimeOfDay();
	}
	return 0;
}

void initializeScheduler()
{
	syncOSCOffsetWithTimeOfDay();
	
	pthread_t resyncThread;
	pthread_create (&resyncThread, NULL, resyncThreadFunc, (void*)0);
	set_real_time_priority(resyncThread);
}
#endif // SC_AUDIO_API_COREAUDIO

// =====================================================================
// Timing (PortAudio)

#if SC_AUDIO_API == SC_AUDIO_API_PORTAUDIO

int64 gOSCoffset = 0; 

static inline int64 GetCurrentOSCTime()
{
	struct timeval tv;
	uint64 s, f;
#ifdef SC_WIN32
	win32_gettimeofday(&tv, 0);
#else
	gettimeofday(&tv, 0);
#endif
	s = (uint64)tv.tv_sec + (uint64)kSECONDS_FROM_1900_to_1970;
	f = (uint64)((double)tv.tv_usec * kMicrosToOSCunits);

	return (s << 32) + f;
}

int32 server_timeseed()
{
	int64 time = GetCurrentOSCTime();
	return Hash((int32)(time >> 32) + Hash((int32)time));
}

int64 oscTimeNow()
{
	return GetCurrentOSCTime();
}

void initializeScheduler()
{
	gOSCoffset = GetCurrentOSCTime(); 
}
#endif // SC_AUDIO_API_PORTAUDIO


// =====================================================================
// Packets (Common)

bool ProcessOSCPacket(World *inWorld, OSC_Packet *inPacket);
void PerformOSCBundle(World *inWorld, OSC_Packet *inPacket);
int PerformOSCMessage(World *inWorld, int inSize, char *inData, ReplyAddress *inReply);

void Perform_ToEngine_Msg(FifoMsg *inMsg);
void FreeOSCPacket(FifoMsg *inMsg);

struct IsBundle
{
	IsBundle() { str4cpy(s, "#bundle"); }
	bool checkIsBundle(int32 *in) { return in[0] == s[0] && in[1] == s[1]; }
	int32 s[2];
};
IsBundle gIsBundle;

bool ProcessOSCPacket(World *inWorld, OSC_Packet *inPacket)
{
	//scprintf("ProcessOSCPacket %d, '%s'\n", inPacket->mSize, inPacket->mData);
	if (!inPacket) return false;
	bool result;	
	inWorld->mDriverLock->Lock();
		SC_AudioDriver *driver = AudioDriver(inWorld);
		if (!driver) return false;
		inPacket->mIsBundle = gIsBundle.checkIsBundle((int32*)inPacket->mData);
		FifoMsg fifoMsg;
		fifoMsg.Set(inWorld, Perform_ToEngine_Msg, FreeOSCPacket, (void*)inPacket);
		result = driver->SendMsgToEngine(fifoMsg);
	inWorld->mDriverLock->Unlock();
	return result;
}

int PerformOSCMessage(World *inWorld, int inSize, char *inData, ReplyAddress *inReply)
{
	//scprintf("->PerformOSCMessage %d\n", inData[0]);
	SC_LibCmd *cmdObj;
	int cmdNameLen;
	if (inData[0] == 0) {
		cmdNameLen = 4;
		uint32 index = inData[3];
		if (index >= NUMBER_OF_COMMANDS) cmdObj = 0;
		else cmdObj = gCmdArray[index];
	} else {
		cmdNameLen = OSCstrlen(inData);
		cmdObj = gCmdLib->Get((int32*)inData);
	}
	if (!cmdObj) {
		CallSendFailureCommand(inWorld, inData, "Command not found", inReply);
		scprintf("FAILURE %s Command not found\n", inData);
		return kSCErr_NoSuchCommand;
	}
	
	int err = cmdObj->Perform(inWorld, inSize - cmdNameLen, inData + cmdNameLen, inReply);
	//scprintf("<-PerformOSCMessage %d\n", inData[0]);
	return err;
}

void PerformOSCBundle(World *inWorld, OSC_Packet *inPacket)
{
	//scprintf("->PerformOSCBundle %d\n", inPacket->mSize);
	char *data = inPacket->mData + 16;
	char* dataEnd = inPacket->mData + inPacket->mSize;
	while (data < dataEnd) {
		int32 msgSize = ntohl(*(int32*)data);
		data += sizeof(int32);
		//scprintf("msgSize %d\n", msgSize);
		PerformOSCMessage(inWorld, msgSize, data, &inPacket->mReplyAddr);
		data += msgSize;
	}
	//scprintf("<-PerformOSCBundle %d\n", inPacket->mSize);
}

////////////////////////////////////////////////////////////////////////////

void Perform_ToEngine_Msg(FifoMsg *inMsg)
{
	World *world = inMsg->mWorld;
	OSC_Packet *packet = (OSC_Packet*)inMsg->mData;
	if (!packet) return;
	
	SC_AudioDriver *driver = inMsg->mWorld->hw->mAudioDriver;
	
	if (!packet->mIsBundle) {
		PerformOSCMessage(world, packet->mSize, packet->mData, &packet->mReplyAddr);
	} else {
	
		// in real time engine, schedule the packet
		int64 time = OSCtime(packet->mData + 8);

		if (time == 0 || time == 1) {
			PerformOSCBundle(world, packet);
		} else {
			if (time < driver->mOSCbuftime) {
				double seconds = (driver->mOSCbuftime - time)*kOSCtoSecs;
				scprintf("late %.9f\n", seconds);
				//FifoMsg outMsg;
				
				//ReportLateness(packet->mReply, seconds)
			}
			
			SC_ScheduledEvent event(world, time, packet);
			driver->AddEvent(event);
			inMsg->mData = 0;
			inMsg->mFreeFunc = 0;
		}
	}
}


void PerformCompletionMsg(World *inWorld, OSC_Packet *inPacket);
void PerformCompletionMsg(World *inWorld, OSC_Packet *inPacket)
{
	bool isBundle = gIsBundle.checkIsBundle((int32*)inPacket->mData);
	
	if (isBundle) {
		PerformOSCBundle(inWorld, inPacket);
	} else {
		PerformOSCMessage(inWorld, inPacket->mSize, inPacket->mData, &inPacket->mReplyAddr);
	}
}


void FreeOSCPacket(FifoMsg *inMsg)
{
	OSC_Packet *packet = (OSC_Packet*)inMsg->mData;
	if (packet) {
		inMsg->mData = 0;
#ifdef SC_WIN32
#pragma message("$$$todo fixme hack for the 'uninitialized packet->mData ptr when using MSVC 7.1 debug")
    if (packet->mData != reinterpret_cast<char*>(0xcdcdcdcd))
  		free(packet->mData);
#else //#ifdef SC_WIN32
    free(packet->mData);
#endif //#ifdef SC_WIN32
		free(packet);
	}
}

void Free_FromEngine_Msg(FifoMsg *inMsg);
void Free_FromEngine_Msg(FifoMsg *inMsg)
{
	World_Free(inMsg->mWorld, inMsg->mData);
}

// =====================================================================
// Audio driver (Common)

SC_AudioDriver::SC_AudioDriver(struct World *inWorld)
		: mWorld(inWorld)
        , mSampleTime(0)
        , mNumSamplesPerCallback(0)

{
}

SC_AudioDriver::~SC_AudioDriver()
{
    mRunThreadFlag = false;
	mAudioSync.Signal();
	pthread_join(mThread, 0);
}

void* audio_driver_thread_func(void* arg);
void* audio_driver_thread_func(void* arg)
{
	SC_AudioDriver *ca = (SC_AudioDriver*)arg;
	void* result = ca->RunThread();
	return result;
}

void* SC_AudioDriver::RunThread()
{
	TriggersFifo *trigfifo = &mWorld->hw->mTriggers;
	NodeEndsFifo *nodeendfifo = &mWorld->hw->mNodeEnds;
	DeleteGraphDefsFifo *deletegraphfifo = &mWorld->hw->mDeleteGraphDefs;

	while (mRunThreadFlag) {
		// wait for sync
		mAudioSync.WaitNext();
		
		mWorld->mNRTLock->Lock();
		
		// send /tr messages
		trigfifo->Perform();		
		
		// send node status messages
		nodeendfifo->Perform();
		
		// free GraphDefs
		deletegraphfifo->Perform();
		
		// perform messages
		mFromEngine.Perform();

		mWorld->mNRTLock->Unlock();
	}
	return 0;
}	

bool SC_AudioDriver::SendMsgFromEngine(FifoMsg& inMsg)
{
	return mFromEngine.Write(inMsg);
}


bool SC_AudioDriver::SendMsgToEngine(FifoMsg& inMsg)
{
	mToEngine.Free();
	return mToEngine.Write(inMsg);
}

void SC_ScheduledEvent::Perform()
{
	PerformOSCBundle(mWorld, mPacket); 
	FifoMsg msg;
	msg.Set(mWorld, FreeOSCPacket, 0, (void*)mPacket);
	mWorld->hw->mAudioDriver->SendMsgFromEngine(msg);
}

bool SC_AudioDriver::Setup()
{
	mRunThreadFlag = true;
	pthread_create (&mThread, NULL, audio_driver_thread_func, (void*)this);
	set_real_time_priority(mThread);

	int numSamples;
	double sampleRate;

	if (!DriverSetup(&numSamples, &sampleRate)) return false;

	mNumSamplesPerCallback = numSamples;
	//scprintf("mNumSamplesPerCallback %d\n", mNumSamplesPerCallback);
	//scprintf("mHardwareBufferSize %lu\n", mHardwareBufferSize);

	// compute a per sample increment to the OpenSoundControl Time
	mOSCincrementNumerator = (double)mWorld->mBufLength * pow(2.,32.);
	mOSCincrement = (int64)(mOSCincrementNumerator / sampleRate);
	mOSCtoSamples = sampleRate / pow(2.,32.);

	World_SetSampleRate(mWorld, sampleRate);
	mSampleRate = mSmoothSampleRate = sampleRate;
	mBuffersPerSecond = sampleRate / mNumSamplesPerCallback;
	mMaxPeakCounter = (int)mBuffersPerSecond;
	
#ifndef NDEBUG
	scprintf("SC_AudioDriver: numSamples=%d, sampleRate=%f\n", mNumSamplesPerCallback, sampleRate);
#endif

	return true;
}

bool SC_AudioDriver::Start()
{
	mAvgCPU = 0.;
	mPeakCPU = 0.;
	mPeakCounter = 0;
	
	mStartHostSecs = 0.;
	mPrevHostSecs = 0.;
	mStartSampleTime = 0.;
	mPrevSampleTime = 0.;

	World_Start(mWorld);
#ifndef SC_INNERSC
	gStartupOSCTime = oscTimeNow();
#endif //SC_INNERSC

	return DriverStart();
}

bool SC_AudioDriver::Stop()
{
	if (!DriverStop()) return false;
	return true;
}


// =====================================================================
// Audio driver (CoreAudio)

#if SC_AUDIO_API == SC_AUDIO_API_COREAUDIO
SC_CoreAudioDriver::SC_CoreAudioDriver(struct World *inWorld)
		: SC_AudioDriver(inWorld)
        , mInputBufList(0)
{
}

SC_CoreAudioDriver::~SC_CoreAudioDriver()
{
    delete mInputBufList;
}

bool SC_CoreAudioDriver::DriverSetup(int* outNumSamplesPerCallback, double* outSampleRate)
{
	OSStatus	err = kAudioHardwareNoError;
	UInt32	count;
	mOutputDevice = kAudioDeviceUnknown;
	mInputDevice = kAudioDeviceUnknown;

	//scprintf("SC_CoreAudioDriver::Setup world %08X\n", mWorld);

	////////////////////////////////////////////////////////////////////////////////////////////////

	do {
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &count, 0);

		AudioDeviceID *devices = (AudioDeviceID*)malloc(count);
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &count, devices);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioHardwarePropertyDevices error %4.4s\n", (char*)&err);
			free(devices);
			break;
		}
		
		int numdevices = count / sizeof(AudioDeviceID);
		fprintf(stdout, "Number of Devices: %d\n", numdevices);
		for (int i = 0; i < numdevices; ++i) {
			err = AudioDeviceGetPropertyInfo(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, 0);
			if (err != kAudioHardwareNoError) {
				fprintf(stdout, "info kAudioDevicePropertyDeviceName error %4.4s A %d %08X\n", (char*)&err, i, devices[i]);
				break;
			}

			char *name = (char*)malloc(count);
			err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, name);
			if (err != kAudioHardwareNoError) {
				fprintf(stdout, "get kAudioDevicePropertyDeviceName error %4.4s A %d %08X\n", (char*)&err, i, devices[i]);
				free(name);
				break;
			}
			fprintf(stdout, "   %d : \"%s\"\n", i, name);
			free(name);
		}
		free(devices);
		fprintf(stdout, "\n");
	} while (false);
	
	if (mWorld->hw->mDeviceName) {
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &count, 0);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "info kAudioHardwarePropertyDevices error %4.4s\n", (char*)&err);
			return false;
		}

		AudioDeviceID *devices = (AudioDeviceID*)malloc(count);
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &count, devices);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioHardwarePropertyDevices error %4.4s\n", (char*)&err);
			return false;
		}
		
		int numdevices = count / sizeof(AudioDeviceID);
		for (int i = 0; i < numdevices; ++i) {
			err = AudioDeviceGetPropertyInfo(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, 0);
			if (err != kAudioHardwareNoError) {
				fprintf(stdout, "info kAudioDevicePropertyDeviceName error %4.4s B %d %08X\n", (char*)&err, i, devices[i]);
				break;
			}

			char *name = (char*)malloc(count);
			err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, name);
			if (err != kAudioHardwareNoError) {
				fprintf(stdout, "get kAudioDevicePropertyDeviceName error %4.4s B %d %08X\n", (char*)&err, i, devices[i]);
				return false;
			}
			if (strcmp(name, mWorld->hw->mDeviceName) == 0) {
				mOutputDevice = devices[i];
				mInputDevice = mOutputDevice;
				free(name);
				break;
			}
			free(name);
		}
		free(devices);
		if (mOutputDevice == kAudioDeviceUnknown) goto getDefault;
	} else {
		getDefault:

		// get the default output device for the HAL
		count = sizeof(mOutputDevice);		
		//get the output device:
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &count, (void *) & mOutputDevice);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioHardwarePropertyDefaultOutputDevice error %4.4s\n", (char*)&err);
			return false;
		}
		
		//get the input device
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &count, (void *) & mInputDevice);

		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioHardwarePropertyDefaultInputDevice error %4.4s\n", (char*)&err);
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	AudioTimeStamp	now;
	now.mFlags = kAudioTimeStampHostTimeValid;
	now.mHostTime = AudioGetCurrentHostTime();
	
	if (mPreferredHardwareBufferFrameSize)
	{

		count = sizeof(UInt32);	
		err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &mPreferredHardwareBufferFrameSize);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
			//return false;
		}
		if (UseSeparateIO()) 
		{
			count = sizeof(UInt32);	
			err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &mPreferredHardwareBufferFrameSize);
			if (err != kAudioHardwareNoError) {
				fprintf(stdout, "set kAudioDevicePropertyNominalSampleRate error %4.4s\n", (char*)&err);
				//return false;
			}
		}
	}
	
	if (mPreferredSampleRate)
	{
		Float64 sampleRate = mPreferredSampleRate;
		count = sizeof(Float64);	
		err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &sampleRate);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "set kAudioDevicePropertyNominalSampleRate error %4.4s\n", (char*)&err);
			//return false;
		}
		if (UseSeparateIO()) 
		{
			count = sizeof(Float64);	
			err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &sampleRate);
			if (err != kAudioHardwareNoError) {
				fprintf(stdout, "set kAudioDevicePropertyNominalSampleRate error %4.4s\n", (char*)&err);
				//return false;
			}
		}
	}
	
	// get the buffersize that the device uses for IO
	count = sizeof(mHardwareBufferSize);	
	err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyBufferSize, &count, &mHardwareBufferSize);
	if (err != kAudioHardwareNoError) {
		fprintf(stdout, "get kAudioDevicePropertyBufferSize error %4.4s\n", (char*)&err);
		return false;
	}
	//fprintf(stdout, "mHardwareBufferSize = %ld\n", mHardwareBufferSize);
	
	// get a description of the data format used by the output device
	count = sizeof(AudioStreamBasicDescription);	
	err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyStreamFormat, &count, &outputStreamDesc);
	if (err != kAudioHardwareNoError) {
		fprintf(stdout, "get kAudioDevicePropertyStreamFormat error %4.4s\n", (char*)&err);
		return false;
	}

	if (mInputDevice != kAudioDeviceUnknown) {
		// get a description of the data format used by the input device
		count = sizeof(AudioStreamBasicDescription);	
		err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamFormat, &count, &inputStreamDesc);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyStreamFormat error %4.4s\n", (char*)&err);
			return false;
		}
	
		count = sizeof(AudioStreamBasicDescription);	
		err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamFormat, &count, &inputStreamDesc);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyStreamFormat error %4.4s\n", (char*)&err);
			return false;
		}
		if (inputStreamDesc.mSampleRate != outputStreamDesc.mSampleRate) {
			fprintf(stdout, "input and output sample rates do not match. %g != %g\n", inputStreamDesc.mSampleRate, outputStreamDesc.mSampleRate);
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	do {
		err = AudioDeviceGetPropertyInfo(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &count, 0);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "info kAudioDevicePropertyDeviceName error %4.4s C %08X\n", (char*)&err, mInputDevice);
			break;
		}

		char *name = (char*)malloc(count);
		err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &count, name);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyDeviceName error %4.4s C %08X\n", (char*)&err, mInputDevice);
			free(name);
			break;
		}

		fprintf(stdout, "\"%s\" Input Device\n", name);
		free(name);
		
		Boolean writeable;
		err = AudioDeviceGetPropertyInfo(mInputDevice, 0, 0, kAudioDevicePropertyStreamConfiguration,
										 &count, &writeable);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "info kAudioDevicePropertyStreamConfiguration error %4.4s\n", (char*)&err);
			break;
		}

		AudioBufferList *bufList = (AudioBufferList*)malloc(count);
		err = AudioDeviceGetProperty(mInputDevice, 0, 0, kAudioDevicePropertyStreamConfiguration,
									 &count, bufList);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyStreamConfiguration error %4.4s\n", (char*)&err);
			free(bufList);
			break;
		}

		fprintf(stdout, "   Streams: %d\n", bufList->mNumberBuffers);
		for (unsigned int j = 0; j < bufList->mNumberBuffers; ++j) {
			fprintf(stdout, "      %d  channels %d\n", j, bufList->mBuffers[j].mNumberChannels);
		}
		
		free(bufList);
	} while (false);
	fprintf(stdout, "\n");
	
	////////////////////////////////////////////////////////////////////////////////////////////////

	do {
		err = AudioDeviceGetPropertyInfo(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &count, 0);

		char *name = (char*)malloc(count);
		err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &count, name);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyDeviceName error %4.4s\n", (char*)&err);
			free(name);
			break;
		}

		fprintf(stdout, "\"%s\" Output Device\n", name);
		free(name);

		Boolean writeable;
		err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, 1, kAudioDevicePropertyStreamConfiguration,
										 &count, &writeable);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "info kAudioDevicePropertyStreamConfiguration error %4.4s\n", (char*)&err);
			break;
		}

		AudioBufferList *bufList = (AudioBufferList*)malloc(count);
		err = AudioDeviceGetProperty(mOutputDevice, 0, 1, kAudioDevicePropertyStreamConfiguration,
									 &count, bufList);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyStreamConfiguration error %4.4s\n", (char*)&err);
			free(bufList);
			break;
		}

		fprintf(stdout, "   Streams: %d\n", bufList->mNumberBuffers);
		for (unsigned int j = 0; j < bufList->mNumberBuffers; ++j) {
			fprintf(stdout, "      %d  channels %d\n", j, bufList->mBuffers[j].mNumberChannels);
		}
		free(bufList);
	} while (false);
	fprintf(stdout, "\n");

	////////////////////////////////////////////////////////////////////////////////////////////////


	if (UseSeparateIO()) {
		count = sizeof(UInt32);
		err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertySafetyOffset, &count, &mSafetyOffset);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertySafetyOffset error %4.4s\n", (char*)&err);
			return false;
		}
		scprintf("mSafetyOffset %lu\n", mSafetyOffset);
	
		Boolean writeable;
		err = AudioDeviceGetPropertyInfo(mInputDevice, 0, true, kAudioDevicePropertyStreamConfiguration, &count, &writeable);
		mInputBufList = (AudioBufferList*)malloc(count);
		err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamConfiguration, &count, mInputBufList);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyStreamConfiguration error %4.4s\n", (char*)&err);
			return false;
		}
	
		scprintf("mNumberBuffers %lu\n", mInputBufList->mNumberBuffers);
		for (uint32 i=0; i<mInputBufList->mNumberBuffers; ++i) {
			scprintf("  mDataByteSize %d %lu\n", i, mInputBufList->mBuffers[i].mDataByteSize);
			mInputBufList->mBuffers[i].mData = zalloc(1, mInputBufList->mBuffers[i].mDataByteSize);
		}
	
	
		AudioTimeStamp	now;
		now.mFlags = kAudioTimeStampHostTimeValid;
		now.mHostTime = AudioGetCurrentHostTime();
		
		err = AudioDeviceSetProperty(mInputDevice, &now, 0, true, kAudioDevicePropertyRegisterBufferList, count, mInputBufList);
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "get kAudioDevicePropertyRegisterBufferList error %4.4s\n", (char*)&err);
			return false;
		}
	}

	*outNumSamplesPerCallback = mHardwareBufferSize / outputStreamDesc.mBytesPerFrame;
	*outSampleRate = outputStreamDesc.mSampleRate;

	return true;
}

// this is the audio processing callback for two separate devices.
OSStatus appIOProc2 (AudioDeviceID inDevice, const AudioTimeStamp* inNow, 
					const AudioBufferList* inInputData, 
					const AudioTimeStamp* inInputTime, 
					AudioBufferList* outOutputData, 
					const AudioTimeStamp* inOutputTime,
					void* defptr);
OSStatus appIOProc2 (AudioDeviceID /*inDevice*/, const AudioTimeStamp* inNow, 
					const AudioBufferList* inInputData,
					const AudioTimeStamp* /*inInputTime*/, 
					AudioBufferList* outOutputData, 
					const AudioTimeStamp* inOutputTime,
					void* defptr)
{
	SC_CoreAudioDriver* def = (SC_CoreAudioDriver*)defptr;

	int64 oscTime = CoreAudioHostTimeToOSC(inOutputTime->mHostTime);
	
	AudioTimeStamp readTime;
	readTime.mSampleTime = inNow->mSampleTime - def->SafetyOffset() - def->NumSamplesPerCallback();
	readTime.mFlags = kAudioTimeStampSampleTimeValid;
	
	AudioDeviceRead(def->InputDevice(), &readTime, def->GetInputBufferList());
	
	def->Run(def->GetInputBufferList(), outOutputData, oscTime);

	return kAudioHardwareNoError;
}


// this is the audio processing callback for one device.
OSStatus appIOProc (AudioDeviceID inDevice, const AudioTimeStamp* inNow, 
					const AudioBufferList* inInputData, 
					const AudioTimeStamp* inInputTime, 
					AudioBufferList* outOutputData, 
					const AudioTimeStamp* inOutputTime,
					void* defptr);
OSStatus appIOProc (AudioDeviceID /*inDevice*/, const AudioTimeStamp* inNow, 
					const AudioBufferList* inInputData,
					const AudioTimeStamp* inInputTime, 
					AudioBufferList* outOutputData, 
					const AudioTimeStamp* inOutputTime,
					void* defptr)
{
	SC_CoreAudioDriver* def = (SC_CoreAudioDriver*)defptr;

	int64 oscTime = CoreAudioHostTimeToOSC(inOutputTime->mHostTime);
	
	double hostSecs = (double)AudioConvertHostTimeToNanos(inOutputTime->mHostTime) * 1e-9;
	double sampleTime = inOutputTime->mSampleTime;
	if (def->mStartHostSecs == 0) {
		def->mStartHostSecs = hostSecs;
		def->mStartSampleTime = sampleTime;
	} else {
		double instSampleRate = (sampleTime -  def->mPrevSampleTime)/(hostSecs -  def->mPrevHostSecs);
		double smoothSampleRate = def->mSmoothSampleRate;
		smoothSampleRate = smoothSampleRate + 0.002 * (instSampleRate - smoothSampleRate);
		def->mOSCincrement = (int64)(def->mOSCincrementNumerator / smoothSampleRate);
		def->mSmoothSampleRate = smoothSampleRate;

#if 0
		double avgSampleRate  = (sampleTime - def->mStartSampleTime)/(hostSecs - def->mStartHostSecs);
		double jitter = (smoothSampleRate * (hostSecs - def->mPrevHostSecs)) - (sampleTime - def->mPrevSampleTime);
		double drift = (smoothSampleRate - def->mSampleRate) * (hostSecs - def->mStartHostSecs);
		//if (fabs(jitter) > 0.01) {
			scprintf("avgSR %.6f   smoothSR %.6f   instSR %.6f   jitter %.6f   drift %.6f   inc %lld\n", 
				avgSampleRate, smoothSampleRate, instSampleRate, jitter, drift, def->mOSCincrement);
		//}
#endif
	}
	def->mPrevHostSecs = hostSecs;
	def->mPrevSampleTime = sampleTime;

	def->Run(inInputData, outOutputData, oscTime);

	return kAudioHardwareNoError;
}

void SC_CoreAudioDriver::Run(const AudioBufferList* inInputData, 
					AudioBufferList* outOutputData, int64 oscTime)
{
	int64 systemTimeBefore = AudioGetCurrentHostTime();
	World *world = mWorld;
	
	try {
		int numSamplesPerCallback = NumSamplesPerCallback();
		mOSCbuftime = oscTime;
		
		mFromEngine.Free();
		/*if (mToEngine.HasData()) {
			scprintf("oscTime %.9f %.9f\n", oscTime*kOSCtoSecs, CoreAudioHostTimeToOSC(AudioGetCurrentHostTime())*kOSCtoSecs);
		}*/
		mToEngine.Perform();
		
		int bufFrames = world->mBufLength;
		int numBufs = numSamplesPerCallback / bufFrames;
		
		int numInputBuses    = world->mNumInputs;
		int numOutputBuses   = world->mNumOutputs;
		float* inputBuses    = world->mAudioBus + world->mNumOutputs * bufFrames;
		float* outputBuses   = world->mAudioBus;
		int32* inputTouched  = world->mAudioBusTouched + world->mNumOutputs;
		int32* outputTouched = world->mAudioBusTouched;
		int numInputStreams  = inInputData ? inInputData->mNumberBuffers : 0;
		int numOutputStreams = outOutputData ? outOutputData->mNumberBuffers : 0;
		
		//static int go = 0;
		
		int64 oscInc = mOSCincrement;
		double oscToSamples = mOSCtoSamples;
	
		int bufFramePos = 0;
		
		for (int i = 0; i < numBufs; ++i, world->mBufCounter++, bufFramePos += bufFrames) {
			int32 bufCounter = world->mBufCounter;
			
			// de-interleave input
			if (inInputData) {
				const AudioBuffer* inInputDataBuffers = inInputData->mBuffers;
				for (int s = 0, b = 0; b<numInputBuses && s < numInputStreams; s++) {
					const AudioBuffer* buf = inInputDataBuffers + s;
					int nchan = buf->mNumberChannels;
					if (buf->mData) {
						float *busdata = inputBuses + b * bufFrames;
						float *bufdata = (float*)buf->mData + bufFramePos * nchan;
						if (nchan == 1) {
							for (int k=0; k<bufFrames; ++k) {
								busdata[k] = bufdata[k];
							}
							inputTouched[b] = bufCounter;
						} else {
							int minchan = sc_min(nchan, numInputBuses - b);
							for (int j=0; j<minchan; ++j, busdata += bufFrames) {
								for (int k=0, m=j; k<bufFrames; ++k, m += nchan) {
									busdata[k] = bufdata[m];
								}
								inputTouched[b+j] = bufCounter;
							}
						}
						b += nchan;
					}
				}
			}
			//count++;
					
			
			int64 schedTime;
			int64 nextTime = oscTime + oscInc;
			
			/*if (mScheduler.Ready(nextTime)) {
				double diff = (mScheduler.NextTime() - mOSCbuftime)*kOSCtoSecs; 
				scprintf("rdy %.9f %.9f %.9f\n", (mScheduler.NextTime()-gStartupOSCTime) * kOSCtoSecs, (mOSCbuftime-gStartupOSCTime)*kOSCtoSecs, diff);
			}*/
			
			while ((schedTime = mScheduler.NextTime()) <= nextTime) {
				float diffTime = (float)(schedTime - oscTime) * oscToSamples + 0.5;
				float diffTimeFloor = floor(diffTime);
				world->mSampleOffset = (int)diffTimeFloor;
				world->mSubsampleOffset = diffTime - diffTimeFloor;
				
				if (world->mSampleOffset < 0) world->mSampleOffset = 0;
				else if (world->mSampleOffset >= world->mBufLength) world->mSampleOffset = world->mBufLength-1;
				
				SC_ScheduledEvent event = mScheduler.Remove();
				event.Perform();
			}
			world->mSampleOffset = 0;
			world->mSubsampleOffset = 0.f;
			
			World_Run(world);
	
			// interleave output
			AudioBuffer* outOutputDataBuffers = outOutputData->mBuffers;
			for (int s = 0, b = 0; b<numOutputBuses && s < numOutputStreams; s++) {
				AudioBuffer* buf = outOutputDataBuffers + s;
				int nchan = buf->mNumberChannels;
				if (buf->mData) {
					float *busdata = outputBuses + b * bufFrames;
					float *bufdata = (float*)buf->mData + bufFramePos * nchan;
					if (nchan == 1) {
						if (outputTouched[b] == bufCounter) {
							for (int k=0; k<bufFrames; ++k) {
								bufdata[k] = busdata[k];
							}
						} 
					} else {
						int minchan = sc_min(nchan, numOutputBuses - b);
						for (int j=0; j<minchan; ++j, busdata += bufFrames) {
							if (outputTouched[b+j] == bufCounter) {
								for (int k=0, m=j; k<bufFrames; ++k, m += nchan) {
									bufdata[m] = busdata[k];
								}
							} 
						}
					}
					b += nchan;
				}
			}	
			oscTime = mOSCbuftime = nextTime;
		}
	} catch (std::exception& exc) {
		scprintf("exception in real time: %s\n", exc.what());
	} catch (...) {
		scprintf("unknown exception in real time\n");
	}
	int64 systemTimeAfter = AudioGetCurrentHostTime();
	double calcTime = (double)AudioConvertHostTimeToNanos(systemTimeAfter - systemTimeBefore) * 1e-9;
	double cpuUsage = calcTime * mBuffersPerSecond * 100.;
	mAvgCPU = mAvgCPU + 0.1 * (cpuUsage - mAvgCPU);
	if (cpuUsage > mPeakCPU || --mPeakCounter <= 0)
	{
		mPeakCPU = cpuUsage;
		mPeakCounter = mMaxPeakCounter;
	}
	
	mAudioSync.Signal();
}

bool SC_CoreAudioDriver::DriverStart()
{
	OSStatus	err = kAudioHardwareNoError;
	AudioTimeStamp	now;
	UInt32 propertySize;
	Boolean writable;
	
	scprintf("start   UseSeparateIO?: %d\n", UseSeparateIO());
	
	if (UseSeparateIO()) {
		err = AudioDeviceAddIOProc(mOutputDevice, appIOProc2, (void *) this);	// setup our device with an IO proc
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "AudioDeviceAddIOProc failed %d\n", (int)err);
			return false;
		}

		if (mWorld->hw->mOutputStreamsEnabled) {
			err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
			AudioHardwareIOProcStreamUsage *su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
			su->mIOProc = (void*)appIOProc2;
			err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, su);
			int len = std::min(su->mNumberStreams, strlen(mWorld->hw->mOutputStreamsEnabled));
			for (int i=0; i<len; ++i) {
				su->mStreamIsOn[i] = mWorld->hw->mOutputStreamsEnabled[i] == '1';
			}
			err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyIOProcStreamUsage, propertySize, su);
		}
		
		err = AudioDeviceStart(mInputDevice, NULL);		// start playing sound through the device
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "AudioDeviceStart failed %d\n", (int)err);
			return false;
		}
		
		err = AudioDeviceStart(mOutputDevice, appIOProc2);		// start playing sound through the device
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "AudioDeviceStart failed %d\n", (int)err);
			err = AudioDeviceStop(mInputDevice, NULL);		// stop playing sound through the device
			return false;
		}
	} else {
		err = AudioDeviceAddIOProc(mOutputDevice, appIOProc, (void *) this);	// setup our device with an IO proc
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "AudioDeviceAddIOProc failed %d\n", (int)err);
			return false;
		}

		if (mWorld->hw->mInputStreamsEnabled) {
			err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
			AudioHardwareIOProcStreamUsage *su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
			su->mIOProc = (void*)appIOProc;
			err = AudioDeviceGetProperty(mOutputDevice, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, su);
			int len = std::min(su->mNumberStreams, strlen(mWorld->hw->mInputStreamsEnabled));
			for (int i=0; i<len; ++i) {
				su->mStreamIsOn[i] = mWorld->hw->mInputStreamsEnabled[i] == '1';
			}
			err = AudioDeviceSetProperty(mOutputDevice, &now, 0, true, kAudioDevicePropertyIOProcStreamUsage, propertySize, su);
		}
		
		if (mWorld->hw->mOutputStreamsEnabled) {
			err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
			AudioHardwareIOProcStreamUsage *su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
			su->mIOProc = (void*)appIOProc;
			err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, su);
			int len = std::min(su->mNumberStreams, strlen(mWorld->hw->mOutputStreamsEnabled));
			for (int i=0; i<len; ++i) {
				su->mStreamIsOn[i] = mWorld->hw->mOutputStreamsEnabled[i] == '1';
			}
			err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyIOProcStreamUsage, propertySize, su);
		}
	
		err = AudioDeviceStart(mOutputDevice, appIOProc);		// start playing sound through the device
		if (err != kAudioHardwareNoError) {
			fprintf(stdout, "AudioDeviceStart failed %d\n", (int)err);
			return false;
		}
	}

	return true;
}

bool SC_CoreAudioDriver::DriverStop()
{
	OSStatus err = kAudioHardwareNoError;

	if (UseSeparateIO()) {
		err = AudioDeviceStop(mOutputDevice, appIOProc2);		
		if (err != kAudioHardwareNoError) return false;
	
		err = AudioDeviceRemoveIOProc(mOutputDevice, appIOProc2);	
		if (err != kAudioHardwareNoError) return false;
	} else {
		err = AudioDeviceStop(mOutputDevice, appIOProc);		
		if (err != kAudioHardwareNoError) return false;
	
		err = AudioDeviceRemoveIOProc(mOutputDevice, appIOProc);	
		if (err != kAudioHardwareNoError) return false;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// These are not linked in yet, but we'll need to listen for the properties and stop/restart synthesis
// if sample-rate, format, or device change.

OSStatus	hardwareListenerProc (	AudioHardwarePropertyID	inPropertyID,
                                    void*					inClientData)
{
    OSStatus			err = noErr;
    char				cStr[255];
    UInt32				outSize;
    Boolean				outWritable;
    AudioDeviceID		deviceID;
   
    switch(inPropertyID)
    {
        case kAudioHardwarePropertyDefaultOutputDevice:
            fprintf(stdout, "%s\n", "***** HARDWARE NOTIFICATION - kAudioHardwarePropertyDefaultOutputDevice\r");
            err =  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultOutputDevice,  &outSize, &outWritable);
			if (err) break;
			err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &outSize, &deviceID);
			if (err) break;
			err =  AudioDeviceGetPropertyInfo(deviceID, 0, false, kAudioDevicePropertyDeviceName,  &outSize, &outWritable);
			if (err) break;
			err = AudioDeviceGetProperty(deviceID, 0, false, kAudioDevicePropertyDeviceName, &outSize, cStr);
			if (err) break;

			// do something
            
            break;
       
        case kAudioHardwarePropertyDefaultInputDevice:
            fprintf(stdout, "%s\n", "***** HARDWARE NOTIFICATION - kAudioHardwarePropertyDefaultInputDevice\r");
            err =  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultInputDevice,  &outSize, &outWritable);
			if (err) break;
			err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &outSize, &deviceID);
			if (err) break;
			err =  AudioDeviceGetPropertyInfo(deviceID, 0, false, kAudioDevicePropertyDeviceName,  &outSize, &outWritable);
			if (err) break;
			err = AudioDeviceGetProperty(deviceID, 0, false, kAudioDevicePropertyDeviceName, &outSize, cStr);
			if (err) break;

			// do something
            
            break;
            
        case kAudioHardwarePropertyDefaultSystemOutputDevice:
            fprintf(stdout, "%s\n", "***** HARDWARE NOTIFICATION - kAudioHardwarePropertyDefaultSystemOutputDevice\r");
            err =  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultSystemOutputDevice,  &outSize, &outWritable);
			if (err) break;
			err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultSystemOutputDevice, &outSize, &deviceID);
			if (err) break;
			err =  AudioDeviceGetPropertyInfo(deviceID, 0, false, kAudioDevicePropertyDeviceName,  &outSize, &outWritable);
			if (err) break;
			err = AudioDeviceGetProperty(deviceID, 0, false, kAudioDevicePropertyDeviceName, &outSize, cStr);
			if (err) break;

			// do something
            
            break;

        case kAudioHardwarePropertyDevices:
        {
            fprintf(stdout, "%s\n", "***** HARDWARE NOTIFICATION - kAudioHardwarePropertyDevices\r");
        }
            break;
    }
    
    fflush(stdout);
    return (noErr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Listen for Device Properties and update interface abd globals
OSStatus	deviceListenerProc (	AudioDeviceID			inDevice,
									UInt32					inLine,
									Boolean					isInput,
									AudioDevicePropertyID	inPropertyID,
									void*					inClientData)
{
    OSStatus			err = noErr;
    UInt32 				outSize,
                        theUIntData,
						mute, 
						playThru;
    UInt32				tLong;
	Float32				vol;
	
    switch(inPropertyID)
    {
        case kAudioDevicePropertyBufferSize:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyBufferSize\r");
            outSize = sizeof(UInt32);
            err = AudioDeviceGetProperty(inDevice, 0, 0, kAudioDevicePropertyBufferSize, &outSize, &theUIntData);
		
            break;
        
        case kAudioDevicePropertyBufferFrameSize:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyBufferFrameSize\r");
            outSize = sizeof(UInt32);
            err = AudioDeviceGetProperty(inDevice, 0, 0, kAudioDevicePropertyBufferFrameSize, &outSize, &theUIntData);

            break;

       case kAudioDevicePropertyBufferSizeRange: 
        {
            AudioValueRange		range;
            
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyBufferSizeRange\r");
            outSize = sizeof(AudioValueRange);
            err = AudioDeviceGetProperty(inDevice, 0, isInput, kAudioDevicePropertyBufferSizeRange, &outSize, &range);
		}
			break;

        case kAudioDevicePropertyStreamFormat:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyStreamFormat\r");
            break;

        case kAudioDevicePropertyDeviceIsRunning:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyDeviceIsRunning\r");
            outSize = sizeof(UInt32);
            err = AudioDeviceGetProperty(inDevice, inLine, isInput, kAudioDevicePropertyDeviceIsRunning, &outSize, &theUIntData);

             break;

        case kAudioDevicePropertyVolumeScalar:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyVolumeScalar\r");
            outSize = sizeof(Float32);
            err = AudioDeviceGetProperty(inDevice, inLine, isInput, kAudioDevicePropertyVolumeScalar, &outSize, &vol);
            break;

        case kAudioDevicePropertyMute:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyMute\r");
            outSize = sizeof(UInt32);
            err = AudioDeviceGetProperty(inDevice, inLine, isInput, kAudioDevicePropertyMute, &outSize, &mute);
            break;

        case kAudioDevicePropertyPlayThru:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyPlayThru\r");
            outSize = sizeof(UInt32);
            err = AudioDeviceGetProperty(inDevice, inLine, isInput, kAudioDevicePropertyPlayThru, &outSize, &playThru);

            break;

        case kAudioDevicePropertyDeviceIsAlive:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyDeviceIsAlive\r");
            outSize = sizeof(UInt32);
            err = AudioDeviceGetProperty(inDevice, 0, false, kAudioDevicePropertyDeviceIsAlive, &outSize, &tLong);

            break;

        case kAudioDevicePropertyDataSource:
            fprintf(stdout, "%s\n", "***** DEVICE NOTIFICATION - kAudioDevicePropertyDataSource\r");
            // get the source
            // match the source to one of the available sources and return the index of that source
            //SetControlValue(control, (chan->vol) * 100);
            break;
    }
        
    fflush(stdout);
    return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus	streamListenerProc (	AudioStreamID			inStream,
									UInt32					inChannel,
									AudioDevicePropertyID	inPropertyID,
									void*					inClientData)
{
    OSStatus				err = noErr;
            
    switch(inPropertyID)
    {
        case kAudioStreamPropertyPhysicalFormat:
            fprintf(stdout, "%s\n", "***** STREAM NOTIFICATION - kAudioStreamPropertyPhysicalFormat\r");
            break;

        case kAudioDevicePropertyStreamFormat:
            fprintf(stdout, "%s\n", "***** STREAM NOTIFICATION - kAudioDevicePropertyStreamFormat\r");
            break;
            
        default:
            break;
    }
    
    fflush(stdout);

    return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus	AddStreamListeners (AudioDeviceID inDevice, AudioDevicePropertyID	inPropertyID, Boolean isInput, void *inClientData);

OSStatus    AddDeviceListeners(AudioDeviceID inDevice, void *inClientData)
{
    OSStatus		err = noErr;
                
    // kAudioDevicePropertyBufferSize
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyBufferSize, deviceListenerProc, inClientData);
    if (err) return err;

    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyBufferSize, deviceListenerProc, inClientData);
    if (err) return err;


    // kAudioDevicePropertyBufferFrameSize
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyBufferFrameSize, deviceListenerProc, inClientData);
    if (err) return err;

    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyBufferFrameSize, deviceListenerProc, inClientData);
    if (err) return err;

    // kAudioDevicePropertyDeviceIsRunning
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyDeviceIsRunning, deviceListenerProc, inClientData);
    if (err) return err;

    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyDeviceIsRunning, deviceListenerProc, inClientData);
    if (err) return err;

/*
    for (i = 0; i <= deviceInfo->totalOutputChannels; i++)
    {
        // kAudioDevicePropertyVolumeScalar output
        err = AudioDeviceAddPropertyListener(inDevice, i, false, kAudioDevicePropertyVolumeScalar, deviceListenerProc, inClientData);
        if (err) return err;

        // kAudioDevicePropertyVolumeMute output
        err = AudioDeviceAddPropertyListener(inDevice, i, false, kAudioDevicePropertyMute, deviceListenerProc, inClientData);
        if (err) return err;
    }
    
    for (i = 0; i <= deviceInfo->totalInputChannels; i++)
    {
        // kAudioDevicePropertyVolumeScalar input
        err = AudioDeviceAddPropertyListener(inDevice, i, true, kAudioDevicePropertyVolumeScalar, deviceListenerProc, inClientData);
        if (err) return err;

        // kAudioDevicePropertyVolumeMute input
        err = AudioDeviceAddPropertyListener(inDevice, i, true, kAudioDevicePropertyMute, deviceListenerProc, inClientData);
        if (err) return err;

        // kAudioDevicePropertyPlayThru input
        err = AudioDeviceAddPropertyListener(inDevice, i, true, kAudioDevicePropertyPlayThru, deviceListenerProc, inClientData);
        if (err) return err;
    }
*/
    
    // kAudioDevicePropertyDeviceIsAlive
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyDeviceIsAlive, deviceListenerProc, inClientData);
    if (err) return err;

    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyDeviceIsAlive, deviceListenerProc, inClientData);
    if (err) return err;


    // kAudioDevicePropertyStreamFormat
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyStreamFormat, deviceListenerProc, inClientData);
    if (err) return err;
    
    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyStreamFormat, deviceListenerProc, inClientData);
    if (err) return err;

    // kAudioDevicePropertyBufferSizeRange
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyBufferSizeRange, deviceListenerProc, inClientData);
    if (err) return err;
    
    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyBufferSizeRange, deviceListenerProc, inClientData);
    if (err) return err;
        
    //kAudioDevicePropertyDataSource
    err = AudioDeviceAddPropertyListener(inDevice, 0, false, kAudioDevicePropertyDataSource, deviceListenerProc, inClientData);
    if (err) return err;
    
    err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyDataSource, deviceListenerProc, inClientData);
    if (err) return err;

    
    AddStreamListeners (inDevice, kAudioStreamPropertyPhysicalFormat, false, inClientData);
    
    AddStreamListeners (inDevice, kAudioStreamPropertyPhysicalFormat, true, inClientData);
    
    return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus AddHardwareListeners (void* inClientData)
{
    OSStatus			err = noErr;
    
    err = AudioHardwareAddPropertyListener(kAudioHardwarePropertyDefaultOutputDevice, hardwareListenerProc, inClientData);
    if (err) return err;

    err = AudioHardwareAddPropertyListener(kAudioHardwarePropertyDefaultInputDevice, hardwareListenerProc, inClientData);
    if (err) return err;

    err = AudioHardwareAddPropertyListener(kAudioHardwarePropertyDefaultSystemOutputDevice, hardwareListenerProc, inClientData);
    if (err) return err;

    err = AudioHardwareAddPropertyListener(kAudioHardwarePropertyDevices, hardwareListenerProc, inClientData);
    if (err) return err;

    return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus	AddStreamListeners (AudioDeviceID inDevice, AudioDevicePropertyID	inPropertyID, Boolean isInput, void *inClientData)
{
    OSStatus			err = noErr;
    UInt32				count;
    UInt32				outSize;
    Boolean				outWritable;
    AudioStreamID		*streamList = nil;
        
    err =  AudioDeviceGetPropertyInfo(inDevice, 0, isInput, kAudioDevicePropertyStreams,  &outSize, &outWritable);
    if (err == noErr)
    {
        streamList = (AudioStreamID*)malloc(outSize);
        err = AudioDeviceGetProperty(inDevice, 0, isInput, kAudioDevicePropertyStreams, &outSize, streamList);
        if (err == noErr)
        {
            for (count = 0; count < (outSize / sizeof(AudioStreamID)); count++)
            {    
                err = AudioStreamAddPropertyListener(streamList[count], 0, inPropertyID, streamListenerProc, inClientData);
                if (err) return err;
            }
        }
        if (streamList != nil)	
        {
            free(streamList);
            streamList = nil;
        }
    }
        

    return (noErr);
}    
#endif // SC_AUDIO_API_COREAUDIO

// =====================================================================
// Audio driver (PortAudio)

#if SC_AUDIO_API == SC_AUDIO_API_PORTAUDIO

// =====================================================================
// SC_PortAudioDriver (PortAudio)

#define PRINT_PORTAUDIO_ERROR( function, errorcode )\
        scprintf( "SC_PortAudioDriver: PortAudio failed at %s with error: '%s'\n",\
                #function, Pa_GetErrorText( errorcode ) )

SC_PortAudioDriver::SC_PortAudioDriver(struct World *inWorld)
		: SC_AudioDriver(inWorld)
        , mStream(0)
{
    PaError paerror = Pa_Initialize();
    if( paerror != paNoError )
        PRINT_PORTAUDIO_ERROR( Pa_Initialize, paerror );
}

SC_PortAudioDriver::~SC_PortAudioDriver()
{
    if( mStream ){
        PaError paerror = Pa_CloseStream( mStream );
        if( paerror != paNoError )
            PRINT_PORTAUDIO_ERROR( Pa_CloseStream, paerror );
    }
    Pa_Terminate();
}

static int SC_PortAudioStreamCallback( const void *input, void *output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags, void *userData )
{
    SC_PortAudioDriver *driver = (SC_PortAudioDriver*)userData;

    return driver->PortAudioCallback( input, output, frameCount, timeInfo, statusFlags );
}

int64 SC_PortAudioDriver::PortAudioTimeToHostTime(PaTime time) 
{
	int s = (uint64)time;
	int f = (uint64)((time - s)/1.0e-6 * kMicrosToOSCunits);

	return gOSCoffset + ((s << 32) + f);
};

int SC_PortAudioDriver::PortAudioCallback( const void *input, void *output,
            unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags )
{
    World *world = mWorld;

    (void) frameCount, timeInfo, statusFlags; // suppress unused parameter warnings
    
	try {
        int64 oscTime = PortAudioTimeToHostTime(timeInfo->outputBufferDacTime );
		mOSCbuftime = oscTime;
		
		mFromEngine.Free();
		mToEngine.Perform();
		
		int numInputs = mInputChannelCount;
		int numOutputs = mOutputChannelCount;
		const float **inBuffers = (const float**)input;
		float **outBuffers = (float**)output;

		int numSamples = NumSamplesPerCallback();
		int bufFrames = mWorld->mBufLength;
		int numBufs = numSamples / bufFrames;

		float *inBuses = mWorld->mAudioBus + mWorld->mNumOutputs * bufFrames;
		float *outBuses = mWorld->mAudioBus;
		int32 *inTouched = mWorld->mAudioBusTouched + mWorld->mNumOutputs;
		int32 *outTouched = mWorld->mAudioBusTouched;

		int minInputs = std::min<size_t>(numInputs, mWorld->mNumInputs);
		int minOutputs = std::min<size_t>(numOutputs, mWorld->mNumOutputs);

		int bufFramePos = 0;

		int64 oscInc = mOSCincrement;
		double oscToSamples = mOSCtoSamples;
	
		// main loop
		for (int i = 0; i < numBufs; ++i, mWorld->mBufCounter++, bufFramePos += bufFrames)
		{
			int32 bufCounter = mWorld->mBufCounter;
			int32 *tch;
			
			// copy+touch inputs
			tch = inTouched;
			for (int k = 0; k < minInputs; ++k)
			{
				const float *src = inBuffers[k] + bufFramePos;
				float *dst = inBuses + k * bufFrames;
				for (int n = 0; n < bufFrames; ++n) *dst++ = *src++;
				*tch++ = bufCounter;
			}


			// run engine
/*
			int64 schedTime;
			int64 nextTime = oscTime + oscInc;
			while ((schedTime = mScheduler.NextTime()) <= nextTime) {
				world->mSampleOffset = (int)((double)(schedTime - oscTime) * oscToSamples);
				SC_ScheduledEvent event = mScheduler.Remove();
				event.Perform();
				world->mSampleOffset = 0;
			}
*/

            // hack for now, schedule events as soon as they arrive
            
            int64 schedTime;
            int64 nextTime = oscTime + oscInc;
            while ((schedTime = mScheduler.NextTime()) != kMaxInt64) {
                world->mSampleOffset = 0;
				SC_ScheduledEvent event = mScheduler.Remove();
				event.Perform();
				world->mSampleOffset = 0;
			}

			World_Run(world);

			// copy touched outputs
			tch = outTouched;
			for (int k = 0; k < minOutputs; ++k) {
				float *dst = outBuffers[k] + bufFramePos;
				if (*tch++ == bufCounter) {
					float *src = outBuses + k * bufFrames;
					for (int n = 0; n < bufFrames; ++n) *dst++ = *src++;
				} else {
					for (int n = 0; n < bufFrames; ++n) *dst++ = 0.0f;
				}
			}

			// update buffer time
			mOSCbuftime = nextTime;
		}
	} catch (std::exception& exc) {
		scprintf("SC_PortAudioDriver: exception in real time: %s\n", exc.what());
	} catch (...) {
		scprintf("SC_PortAudioDriver: unknown exception in real time\n");
	}

	double cpuUsage = (double)Pa_GetStreamCpuLoad(mStream); 
	mAvgCPU = mAvgCPU + 0.1 * (cpuUsage - mAvgCPU);
	if (cpuUsage > mPeakCPU || --mPeakCounter <= 0)
	{
		mPeakCPU = cpuUsage;
		mPeakCounter = mMaxPeakCounter;
	}

	mAudioSync.Signal();

    return paContinue;
}


// ====================================================================
// NOTE: for now, in lieu of a mechanism that passes generic options to
// the platform driver, we rely on the PortAudio default device environment variables
bool SC_PortAudioDriver::DriverSetup(int* outNumSamples, double* outSampleRate)
{
    char *env;

/*
FIXME:
stefan kersten wrote:
> * the powerbook alsa driver on linux doesn't support input; in the
> jack implementation i took the channel information from
> mWorld->mNumInputs/Outputs (which are set from the command line), i
> believe this should be done for portaudio as well
*/

	// create inputs according to SC_PORTAUDIO_INPUTS (default 2)
	env = getenv("SC_PORTAUDIO_INPUTS");
	if (env == 0) env = "2";
	mInputChannelCount = std::max(2, atoi(env));

	// create outputs according to SC_PORTAUDIO_OUTPUTS (default 2)
	env = getenv("SC_PORTAUDIO_OUTPUTS");
	if (env == 0) env = "2";
	mOutputChannelCount = std::max(2, atoi(env));

  char *env2;
  env = getenv("SC_PORTAUDIO_INPUT_DEVICE");
  env2 = getenv("SC_PORTAUDIO_OUTPUT_DEVICE");
#ifdef SC_WIN32
#ifdef _DEBUG
  if(env)
    printf("SC_PORTAUDIO_INPUT_DEVICE = %s\n",env);
  if(env2)
    printf("SC_PORTAUDIO_OUTPUT_DEVICE= %s\n",env2);
#endif //#ifdef _DEBUG
#endif //#ifdef SC_WIN32
  if (env == 0 && env2 == 0) {
    // we revert to the classic behaviour when no env vars are specified. the latency
    // values are safer when using Pa_OpenDefaultStream than when using 
    // Pa_OpenStream and specifying the default devices.
    *outNumSamples = mWorld->mBufLength;
    *outSampleRate = 44100.; ///$$$TODO fixme use the cmd-line supplied param
    PaError paerror = Pa_OpenDefaultStream( &mStream, mInputChannelCount, mOutputChannelCount,
            paFloat32 | paNonInterleaved, *outSampleRate, *outNumSamples, SC_PortAudioStreamCallback, this );
    if( paerror != paNoError )
        PRINT_PORTAUDIO_ERROR( Pa_OpenDefaultStream, paerror );
    return paerror == paNoError;
  }
  else { 
    // at least one device has been specified. let's retrieve the indices
    PaDeviceIndex inIdx,outIdx;
    if (env == 0) inIdx = Pa_GetDefaultInputDevice( );
    else inIdx = atoi(env);
    if (env2 == 0) outIdx = Pa_GetDefaultOutputDevice( );
    else outIdx = atoi(env2);
    
    PaSampleFormat fmt = paFloat32 | paNonInterleaved;
    
    PaStreamParameters inStreamParams;
    inStreamParams.device = inIdx;
    inStreamParams.channelCount = mInputChannelCount;
    inStreamParams.sampleFormat = fmt;
    inStreamParams.suggestedLatency = Pa_GetDeviceInfo( inIdx )->defaultLowInputLatency; //$$$todo : allow user to choose latency instead of this
    inStreamParams.hostApiSpecificStreamInfo = NULL;

    PaStreamParameters outStreamParams;
    outStreamParams.device = outIdx;
    outStreamParams.channelCount = mOutputChannelCount;
    outStreamParams.sampleFormat = fmt;
    outStreamParams.suggestedLatency = Pa_GetDeviceInfo( outIdx )->defaultLowOutputLatency; //$$$todo : allow user to choose latency instead of this
    outStreamParams.hostApiSpecificStreamInfo = NULL;

    *outNumSamples = mWorld->mBufLength;
    *outSampleRate = 44100.; ///$$$TODO fixme use the cmd-line supplied param
    PaError paerror = Pa_OpenStream(&mStream, &inStreamParams, &outStreamParams, *outSampleRate, *outNumSamples, paNoFlag, SC_PortAudioStreamCallback, this );
    if( paerror != paNoError )
        PRINT_PORTAUDIO_ERROR( Pa_OpenStream, paerror );
    return paerror == paNoError;
  }
}

bool SC_PortAudioDriver::DriverStart()
{
	if (!mStream)
		return false;

    PaError paerror = Pa_StartStream( mStream );
    if( paerror != paNoError )
        PRINT_PORTAUDIO_ERROR( Pa_StartStream, paerror );

	return paerror == paNoError;
}

bool SC_PortAudioDriver::DriverStop()
{
    if (!mStream)
		return false;

    PaError paerror = Pa_StopStream(mStream);
    if( paerror != paNoError )
        PRINT_PORTAUDIO_ERROR( Pa_StopStream, paerror );

	return paerror == paNoError;
}

#endif // SC_AUDIO_API_PORTAUDIO


#if SC_AUDIO_API == SC_AUDIO_API_INNERSC_VST

// =====================================================================
// SC_VSTAudioDriver (VST)


SC_VSTAudioDriver::SC_VSTAudioDriver(struct World *inWorld)
		: SC_AudioDriver(inWorld)
{
  mIsStreaming = false;
  // init big rsrc
}

SC_VSTAudioDriver::~SC_VSTAudioDriver()
{
  // close small rsrc (stream)
  // close big rsrc
}

void SC_VSTAudioDriver::Callback( const void *input, void *output,
            unsigned long frameCount, const VstTimeInfo* timeInfo )
{
    World *world = mWorld;

//    (void) frameCount, timeInfo, statusFlags; // suppress unused parameter warnings
    
	try {
    int64 oscTime = 0; // $$$todo FIXME -> PortAudioTimeToHostTime( mStream, timeInfo.outputBufferDacTime );
		mOSCbuftime = oscTime;
		
		mFromEngine.Free();
		mToEngine.Perform();
		
		int numInputs = mInputChannelCount;
		int numOutputs = mOutputChannelCount;
		const float **inBuffers = (const float**)input;
		float **outBuffers = (float**)output;

		int numSamples = NumSamplesPerCallback();
		int bufFrames = mWorld->mBufLength;
		int numBufs = numSamples / bufFrames;

		float *inBuses = mWorld->mAudioBus + mWorld->mNumOutputs * bufFrames;
		float *outBuses = mWorld->mAudioBus;
		int32 *inTouched = mWorld->mAudioBusTouched + mWorld->mNumOutputs;
		int32 *outTouched = mWorld->mAudioBusTouched;

		int minInputs = std::min<size_t>(numInputs, mWorld->mNumInputs);
		int minOutputs = std::min<size_t>(numOutputs, mWorld->mNumOutputs);

		int bufFramePos = 0;

		int64 oscInc = mOSCincrement;
		double oscToSamples = mOSCtoSamples;
	
		// main loop
		for (int i = 0; i < numBufs; ++i, mWorld->mBufCounter++, bufFramePos += bufFrames) {
			int32 bufCounter = mWorld->mBufCounter;
			int32 *tch;
			// copy+touch inputs
			tch = inTouched;
			for (int k = 0; k < minInputs; ++k) {
				const float *src = inBuffers[k] + bufFramePos;
				float *dst = inBuses + k * bufFrames;
				for (int n = 0; n < bufFrames; ++n)
          *dst++ = *src++;
				*tch++ = bufCounter;
			}
			// run engine
			//int64 schedTime;
			//int64 nextTime = oscTime + oscInc;
			//while ((schedTime = mScheduler.NextTime()) <= nextTime) {
			//	world->mSampleOffset = (int)((double)(schedTime - oscTime) * oscToSamples);
			//	SC_ScheduledEvent event = mScheduler.Remove();
			//	event.Perform();
			//	world->mSampleOffset = 0;
			//}
      // hack for now, schedule events as soon as they arrive
            
      int64 schedTime;
      int64 nextTime = oscTime + oscInc;
      while ((schedTime = mScheduler.NextTime()) != kMaxInt64) {
        world->mSampleOffset = 0;
				SC_ScheduledEvent event = mScheduler.Remove();
				event.Perform();
				world->mSampleOffset = 0;
			}
			World_Run(world);
			// copy touched outputs
			tch = outTouched;
			for (int k = 0; k < minOutputs; ++k) {
				float *dst = outBuffers[k] + bufFramePos;
				if (*tch++ == bufCounter) {
					float *src = outBuses + k * bufFrames;
					for (int n = 0; n < bufFrames; ++n) 
            *dst++ = *src++;
  			} 
        else {
					for (int n = 0; n < bufFrames; ++n) 
            *dst++ = 0.0f;
				}
			}
			// update buffer time
			mOSCbuftime = nextTime;
		}
	} catch (std::exception& exc) {
		scprintf("SC_PortAudioDriver: exception in real time: %s\n", exc.what());
	} catch (...) {
		scprintf("SC_PortAudioDriver: unknown exception in real time\n");
	}

	//double cpuUsage = (double)Pa_GetStreamCpuLoad(mStream); 
  double cpuUsage = 0.0; // $$$todo fix fix. user will check load in host
	mAvgCPU = mAvgCPU + 0.1 * (cpuUsage - mAvgCPU);
	if (cpuUsage > mPeakCPU || --mPeakCounter <= 0)
	{
		mPeakCPU = cpuUsage;
		mPeakCounter = mMaxPeakCounter;
	}
	mAudioSync.Signal();
  //return paContinue;
}


// ====================================================================
// NOTE: for now, in lieu of a mechanism that passes generic options to
// the platform driver, we rely on the PortAudio default device environment variables
bool SC_VSTAudioDriver::DriverSetup(int* outNumSamples, double* outSampleRate)
{
  // should init the driver and write the num of samples per callback
  // and the sample rate in the supplied addresses

  // this should open the resources (and return true if successful), but not 
  // really start the streaming... (this is the resp of DriverStart())
  return true;
}

bool SC_VSTAudioDriver::DriverStart()
{
  return true;
}

bool SC_VSTAudioDriver::DriverStop()
{
  mIsStreaming = false;
  return true;
}

int32 server_timeseed()
{
	static int32 count = 0;
	struct timeval tv;
  double us = timeGetTime( )*1000;
  int sec = us/1000000;
  int usec = us-sec*1000000;
	return (int32)sec ^ (int32)usec ^ count--;
}

static inline int64 GetCurrentOSCTime()
{
  #pragma message("check where GetCurrentOSCTime( ) is called and try to defer that somewhere where VstTimeInfo is available")
  //$$$todo fixme
  return 0;
}


int64 oscTimeNow()
{
	return GetCurrentOSCTime();
}

void initializeScheduler()
{
}

#endif // SC_AUDIO_API_INNERSC_VST
