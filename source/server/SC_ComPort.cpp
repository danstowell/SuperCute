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


#include "SC_ComPort.h"
#include "SC_Lock.h"
#include "SC_HiddenWorld.h"
#include "SC_WorldOptions.h"
#include <ctype.h>
#include <stdexcept>
#include <stdarg.h>
#include <netinet/tcp.h>

#ifdef USE_RENDEZVOUS
#include "Rendezvous.h"
#endif

int recvall(int socket, void *msg, size_t len);
int sendallto(int socket, const void *msg, size_t len, struct sockaddr *toaddr, int addrlen);
int sendall(int socket, const void *msg, size_t len);

void ProcessOSCPacket(World *inWorld, OSC_Packet *inPacket);

void dumpOSCmsg(int inSize, char* inData)
{
	int size;
	char *data;
	
	if (inData[0]) {
		char *addr = inData;
		data = OSCstrskip(inData);
		size = inSize - (data - inData);
		scprintf("[ \"%s\",", addr);
	}
	else
	{
		scprintf("[ %d,", *(int32*)inData);
		data = inData + 4;
		size = inSize - 4;
	}
	
	sc_msg_iter msg(size, data);
				
	while (msg.remain())
	{
		char c = msg.nextTag('i');
		switch(c)
		{
			case 'i' :
				scprintf(" %d", msg.geti());
				break;
			case 'f' :
				scprintf(" %g", msg.getf());
				break;
			case 's' :
				scprintf(" \"%s\"", msg.gets());
				break;
			case 'b' :
				scprintf(" DATA[%d]", msg.getbsize());
				msg.skipb();
				break;
			default :
				scprintf(" !unknown tag '%c' 0x%02x !", isprint(c)?c:'?', (unsigned char)c & 255);
				goto leave;
		}
		if (msg.remain()) scprintf(",");
	}
leave:
	scprintf(" ]");
}

void hexdump(int size, char* data)
{
	char ascii[20];
	int padsize = (size + 15) & -16;
	scprintf("size %d\n", size);
	for (int i=0; i<padsize; ++i)
	{
		if ((i&15)==0)
		{
			scprintf("%4d   ", i);
		}
		if (i >= size) 
		{
			scprintf("   ");
			ascii[i&15] = 0;
		}
		else 
		{
			scprintf("%02x ", (unsigned char)data[i] & 255);
			
			if (isprint(data[i])) ascii[i&15] = data[i];
			else ascii[i&15] = '.';
		}
		if ((i&15)==15)
		{
			ascii[16] = 0;
			scprintf("  |%s|\n", ascii);
		}
		else if ((i&3)==3)
		{
			scprintf(" ");
		}
	}
	scprintf("\n");
}

void dumpOSC(int mode, int size, char* inData)
{
	if (mode & 1)
	{
		if (strcmp(inData, "#bundle") == 0) 
		{
			char* data = inData + 8;
			scprintf("[ \"#bundle\", %lld, ", *(int64*)data);
			data += 8;
			char* dataEnd = inData + size;
			while (data < dataEnd) {
				int32 msgSize = *(int32*)data;
				data += sizeof(int32);
				scprintf("\n    ");
				dumpOSCmsg(msgSize, data);
				data += msgSize;
				if (data < dataEnd) scprintf(",");
			}
			scprintf("\n]\n");
		}
		else 
		{
			dumpOSCmsg(size, inData);
			scprintf("\n");
		}
	}
	
	if (mode & 2) hexdump(size, inData);
}

void World_SendPacket(World *inWorld, int inSize, char *inData, ReplyFunc inFunc)
{
	if (inSize > 0) {
		if (inWorld->mDumpOSC) dumpOSC(inWorld->mDumpOSC, inSize, inData);

		OSC_Packet* packet = (OSC_Packet*)malloc(sizeof(OSC_Packet));
		char *data = (char*)malloc(inSize);
		packet->mReplyAddr.mSockAddr.sin_addr.s_addr = 0;
		packet->mReplyAddr.mSockAddr.sin_port = 0;
		packet->mReplyAddr.mReplyFunc = inFunc;
		packet->mSize = inSize;
		packet->mData = data;
		packet->mReplyAddr.mSocket = 0;
		memcpy(data, inData, inSize);

		ProcessOSCPacket(inWorld, packet);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SC_CmdPort::SC_CmdPort(struct World *inWorld) : mWorld(inWorld)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SC_ComPort::SC_ComPort(struct World *inWorld, int inPortNum)
	: SC_CmdPort(inWorld), mPortNum(inPortNum), mSocket(-1)
{
}

SC_ComPort::~SC_ComPort()
{
    if (mSocket != -1) close(mSocket);
}

void* com_thread_func(void* arg);
void* com_thread_func(void* arg)
{
    SC_CmdPort *thread = (SC_CmdPort*)arg;
    void* result = thread->Run();
    return result;
}

void set_real_time_priority(pthread_t thread);
void set_real_time_priority(pthread_t thread)
{
	int policy;
	struct sched_param param;

	pthread_getschedparam (thread, &policy, &param);
	policy = SCHED_RR;         // round-robin, AKA real-time scheduling
	param.sched_priority = 63; // you'll have to play with this to see what it does
	pthread_setschedparam (thread, policy, &param);
}

void SC_CmdPort::Start()
{
    pthread_create (&mThread, NULL, com_thread_func, (void*)this);
    set_real_time_priority(mThread);
}

#ifdef USE_RENDEZVOUS
void* rendezvous_thread_func(void* arg);
void* rendezvous_thread_func(void* arg)
{
	SC_ComPort* port = reinterpret_cast<SC_ComPort*>(arg);
	port->PublishToRendezvous();
	return NULL;
}

void SC_UdpInPort::PublishToRendezvous()
{
	PublishPortToRendezvous(kSCRendezvous_UDP, htons(mPortNum));
}

void SC_TcpInPort::PublishToRendezvous()
{
	PublishPortToRendezvous(kSCRendezvous_TCP, htons(mPortNum));
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SC_UdpInPort::SC_UdpInPort(struct World *inWorld, int inPortNum)
	: SC_ComPort(inWorld, inPortNum)
{	
	if ((mSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		throw std::runtime_error("failed to create udp socket\n");
	}

	//scprintf("@@@ sizeof(ReplyAddress) %d\n", sizeof(ReplyAddress));

	bzero((char *)&mBindSockAddr, sizeof(mBindSockAddr));
	mBindSockAddr.sin_family = AF_INET;
	mBindSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mBindSockAddr.sin_port = htons(mPortNum);

	if (bind(mSocket, (struct sockaddr *)&mBindSockAddr, sizeof(mBindSockAddr)) < 0) {
		throw std::runtime_error("unable to bind udp socket\n");
	}

	Start();

#ifdef USE_RENDEZVOUS
	pthread_create(&mRendezvousThread, 
		NULL, 
		rendezvous_thread_func, 
		(void*)this);
#endif
}

SC_UdpInPort::~SC_UdpInPort()
{
	if (mSocket != -1) close(mSocket);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void DumpReplyAddress(ReplyAddress *inReplyAddress)
{
	scprintf("mSockAddrLen %d\n", inReplyAddress->mSockAddrLen);
	scprintf("mSocket %d\n", inReplyAddress->mSocket);
	scprintf("mSockAddr.sin_len %d\n", inReplyAddress->mSockAddr.sin_len);
	scprintf("mSockAddr.sin_family %d\n", inReplyAddress->mSockAddr.sin_family);
	scprintf("mSockAddr.sin_port %d\n", inReplyAddress->mSockAddr.sin_port);
	scprintf("mSockAddr.sin_addr.s_addr %d\n", inReplyAddress->mSockAddr.sin_addr.s_addr);
	scprintf("mReplyFunc %08X\n", (uint32)inReplyAddress->mReplyFunc);
}

/*
int32 GetHash(ReplyAddress *inReplyAddress)
{
	int32 hash = Hash(inReplyAddress->mSockAddr.sin_addr.s_addr);
	
	hash += (inReplyAddress->mSockAddr.sin_len << 24)	
		| (inReplyAddress->mSockAddr.sin_family << 16)
		| inReplyAddress->mSockAddr.sin_port;
	
	return Hash(hash);
}

ReplyAddress GetKey(ReplyAddress *inReplyAddress)
{
	return *inReplyAddress
}
*/

bool operator==(const ReplyAddress& a, const ReplyAddress& b)
{
	return a.mSockAddr.sin_addr.s_addr == b.mSockAddr.sin_addr.s_addr
		&& a.mSockAddr.sin_family == b.mSockAddr.sin_family
		&& a.mSockAddr.sin_port == b.mSockAddr.sin_port
		&& a.mSockAddr.sin_len == b.mSockAddr.sin_len
		&& a.mSocket == b.mSocket;
}

void udp_reply_func(struct ReplyAddress *addr, char* msg, int size);
void udp_reply_func(struct ReplyAddress *addr, char* msg, int size)
{
	int total = sendallto(addr->mSocket, msg, size, (sockaddr*)&addr->mSockAddr, addr->mSockAddrLen);
	if (total < size) DumpReplyAddress(addr);
}

ReplyFunc SC_UdpInPort::GetReplyFunc()
{
	return udp_reply_func;
}

void* SC_UdpInPort::Run()
{
	OSC_Packet *packet = 0;
	char *data = 0;
	while (true) {		
		if (!packet) {
			// preallocate packet before we need it.
			packet = (OSC_Packet*)malloc(sizeof(OSC_Packet));
			data = (char*)malloc(kPacketBufSize); // preallocate to maximum
		}
		
		packet->mReplyAddr.mSockAddrLen = sizeof(sockaddr_in);
		int size = recvfrom(mSocket, data, kPacketBufSize , 0,
								(struct sockaddr *) &packet->mReplyAddr.mSockAddr, &packet->mReplyAddr.mSockAddrLen);
		
		if (size > 0) {
			if (mWorld->mDumpOSC) dumpOSC(mWorld->mDumpOSC, size, data);
			
			data = (char*)realloc(data, size); // give back unused portion - a smart malloc should coalesce in place
			packet->mReplyAddr.mReplyFunc = udp_reply_func;
			packet->mSize = size;
			packet->mData = data;
			packet->mReplyAddr.mSocket = mSocket;

			ProcessOSCPacket(mWorld, packet);
			packet = 0;
			data = 0;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SC_TcpInPort::SC_TcpInPort(struct World *inWorld, int inPortNum, int inMaxConnections, int inBacklog)
	: SC_ComPort(inWorld, inPortNum), mConnectionAvailable(inMaxConnections), 
        mBacklog(inBacklog)
{    
    if((mSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("failed to create tcp socket\n");
    }
    
	//const int on = 1;
	//setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    
    bzero((char *)&mBindSockAddr, sizeof(mBindSockAddr));
    mBindSockAddr.sin_family = AF_INET;
    mBindSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mBindSockAddr.sin_port = htons(mPortNum);
    
    if(bind(mSocket, (struct sockaddr *)&mBindSockAddr, sizeof(mBindSockAddr)) < 0)
    {
        throw std::runtime_error("unable to bind tcp socket\n");
    }

    if(listen(mSocket, mBacklog) < 0)
    {
        throw std::runtime_error("unable to listen tcp socket\n");
    }
    
    Start();
#ifdef USE_RENDEZVOUS
	pthread_create(&mRendezvousThread, 
		NULL, 
		rendezvous_thread_func, 
		(void*)this);
#endif
}

void* SC_TcpInPort::Run()
{
    while (true)
    {
        mConnectionAvailable.Acquire();
        struct sockaddr_in address; /* Internet socket address stuct */
        int addressSize=sizeof(struct sockaddr_in);
        int socket = accept(mSocket,(struct sockaddr*)&address,&addressSize);
        if (socket < 0) {
        	mConnectionAvailable.Release();
        } else {
        	new SC_TcpConnectionPort(mWorld, this, socket);
        }
    }
    return 0;
}

void SC_TcpInPort::ConnectionTerminated()
{
        mConnectionAvailable.Release();
}

void null_reply_func(struct ReplyAddress* addr, char* msg, int size);
void null_reply_func(struct ReplyAddress* /*addr*/, char* /*msg*/, int /*size*/)
{
}

ReplyFunc SC_TcpInPort::GetReplyFunc()
{
	return null_reply_func;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SC_TcpConnectionPort::SC_TcpConnectionPort(struct World *inWorld, SC_TcpInPort *inParent, int inSocket)
    : SC_ComPort(inWorld, 0), mParent(inParent)
{
    mSocket = inSocket;
	
	const int on = 1;
	setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
	
    Start();
}

SC_TcpConnectionPort::~SC_TcpConnectionPort()
{
	close(mSocket);
    mParent->ConnectionTerminated();
}

void tcp_reply_func(struct ReplyAddress *addr, char* msg, int size);
void tcp_reply_func(struct ReplyAddress *addr, char* msg, int size)
{
    uint32_t u ;
    // Write size as 32bit signed network-order integer
	u = size;
    u = htonl ( u ) ;
    sendall ( addr->mSocket , &u , 4 ) ;
    // Write message.
    sendall ( addr->mSocket , msg , size ) ;
}


ReplyFunc SC_TcpConnectionPort::GetReplyFunc()
{
	return tcp_reply_func;
}

void* SC_TcpConnectionPort::Run()
{
	char buf[kPacketBufSize];
	OSC_Packet *packet = 0;
	// wait for login message
	int32 size;
	int32 msglen;
	
	// first message must be the password. 4 tries.
	bool validated = mWorld->hw->mPassword[0] == 0;
	for (int i=0; !validated && i<4; ++i) {		
		size = recvall(mSocket, &msglen, sizeof(int32) );
		if (size < 0) goto leave;
		
		size = recvall(mSocket, buf, msglen);
		if (size < 0) goto leave;
		
		validated = strcmp(buf, mWorld->hw->mPassword) == 0;
		if (!validated) sleep(i+1);	// thwart cracking.
	}
	
	if (validated) {
		while (true) {		
			if (!packet) {
				packet = (OSC_Packet*)malloc(sizeof(OSC_Packet));
			}
			size = recvall(mSocket, &msglen, sizeof(int32));
			if (size < 0) goto leave;
			
			char *data = (char*)malloc(msglen);
			size = recvall(mSocket, data, msglen);
			if (size < msglen) goto leave;
			
			if (mWorld->mDumpOSC) dumpOSC(mWorld->mDumpOSC, size, data);
			
			packet->mReplyAddr.mReplyFunc = tcp_reply_func;
			packet->mSize = msglen;
			packet->mData = data;
			packet->mReplyAddr.mSocket = mSocket;
			ProcessOSCPacket(mWorld, packet);
			packet = 0;
		}
	}
leave:
    delete this; // ohh this could cause a crash if a reply tries to access it..
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

int recvall(int socket, void *msg, size_t len)
{
	int total = 0;
	while (total < (int)len)
	{
		int numbytes = recv(socket, msg, len - total, 0);
		if (numbytes <= 0) return total;
		total += numbytes;
		msg = (void*)((char*)msg + numbytes);
	}
	return total;
}

int sendallto(int socket, const void *msg, size_t len, struct sockaddr *toaddr, int addrlen)
{
	int total = 0;
	while (total < (int)len)
	{
		int numbytes = sendto(socket, msg, len - total, 0, toaddr, addrlen);
		if (numbytes < 0) {
			scprintf("sendallto errno %d %s\n", errno, strerror(errno));
			return total;
		}
		total += numbytes;
		msg = (void*)((char*)msg + numbytes);
	}
	return total;
}

int sendall(int socket, const void *msg, size_t len)
{
	int total = 0;
	while (total < (int)len) 
	{
		int numbytes = send(socket, msg, len - total, 0);
		if (numbytes < 0) return total;
		total += numbytes;
		msg = (void*)((char*)msg + numbytes);
	}
	return total;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

