/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#pragma once

#ifdef C4ENGINE
#include "C4Network2.h"
#include "C4Network2Client.h"
#else
#include "C4NetIO.h"
#endif

#include "C4GameParameters.h"
#include "C4Version.h"
#include "C4GameVersion.h"
#include "C4InputValidation.h"

const int C4Network2HTTPQueryTimeout = 20; // (s)

// Session data
class C4Network2Reference
{
public:
	C4Network2Reference();
	~C4Network2Reference();

	// Game parameters
	C4GameParameters Parameters;

private:
	// General information
	int32_t Icon;
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameExNoEmpty> Title;
	C4Network2Status GameStatus;
	int32_t Time;
	int32_t Frame;
	int32_t StartTime;
	int32_t LeaguePerformance; // custom settlement league performance if scenario doesn't use elapsed frames
	ValidatedStdCopyStrBuf<C4InVal::VAL_Comment> Comment;
	bool JoinAllowed;
	bool ObservingAllowed;
	bool PasswordNeeded;
	bool OfficialServer;

	// Engine information
	C4GameVersion Game;

	// Network addresses
	uint8_t iAddrCnt;
	C4Network2Address Addrs[C4ClientMaxAddr];

public:
	const C4Network2Address &getAddr(int i) const { return Addrs[i]; }
	int getAddrCnt() const { return iAddrCnt; }
	const char *getTitle() const { return Title.getData(); }
	int32_t getIcon() const { return Icon; }
	C4Network2Status getGameStatus() const { return GameStatus; }
	const char *getComment() const { return Comment.getData(); }
	const C4GameVersion &getGameVersion() const { return Game; }
	bool isPasswordNeeded() const { return PasswordNeeded; }
	bool isJoinAllowed() const { return JoinAllowed; }
	bool isOfficialServer() const { return OfficialServer; }
	int32_t getSortOrder() const;
	int32_t getTime() const { return Time; }
	int32_t getStartTime() const { return StartTime; }

	void SetSourceIP(in_addr ip);

	void InitLocal(C4Game *pGame);

	void CompileFunc(StdCompiler *pComp);
};

// Serves references (mini-HTTP-server)
class C4Network2RefServer : public C4NetIOTCP
{
public:
	C4Network2RefServer();
	virtual ~C4Network2RefServer();

private:
	CStdCSec RefCSec;
	C4Network2Reference *pReference;

public:
	void Clear();
	void SetReference(C4Network2Reference *pReference);

protected:
	// Overridden
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr);

private:
	// Responses
	void RespondNotImplemented(const C4NetIO::addr_t &addr, const char *szMessage);
	void RespondReference(const C4NetIO::addr_t &addr);
};

// mini HTTP client
class C4Network2HTTPClient : public C4NetIOTCP, private C4NetIO::CBClass
{
public:
	C4Network2HTTPClient();
	virtual ~C4Network2HTTPClient();

private:
	// Address information
	C4NetIO::addr_t ServerAddr, PeerAddr;
	StdCopyStrBuf Server, RequestPath;

	bool fBinary;
	bool fBusy, fSuccess, fConnected;
	size_t iDataOffset;
	StdCopyBuf Request;
	time_t iRequestTimeout;

	// Response header data
	size_t iDownloadedSize, iTotalSize;
	bool fCompressed;

	// Event queue to use for notify when something happens
	class C4InteractiveThread *pNotify;

protected:
	StdCopyBuf ResultBin; // set if fBinary
	StdCopyStrBuf ResultString; // set if !fBinary

protected:
	// Overridden
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr);

	// Callbacks
	bool OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO);
	void OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason);
	void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

	void ResetRequestTimeout();
	virtual int32_t GetDefaultPort() { return 80; }

public:
	bool Query(const StdBuf &Data, bool fBinary);
	bool Query(const char *szData, bool fBinary) { return Query(StdBuf(szData, SLen(szData)), fBinary); }

	bool isBusy() const { return fBusy; }
	bool isSuccess() const { return fSuccess; }
	bool isConnected() const { return fConnected; }
	size_t getTotalSize() const { return iTotalSize; }
	size_t getDownloadedSize() const { return iDownloadedSize; }
	const StdBuf &getResultBin() const { assert(fBinary); return ResultBin; }
	const char *getServerName() const { return Server.getData(); }
	const char *getRequest() const { return RequestPath.getData(); }
	const C4NetIO::addr_t &getServerAddress() const { return ServerAddr; }

	void Cancel(const char *szReason);
	void Clear();

	bool SetServer(const char *szServerAddress);

	void SetNotify(class C4InteractiveThread *pnNotify) { pNotify = pnNotify; }

	// Overridden
	virtual bool Execute(int iMaxTime = TO_INF);
	virtual int GetTimeout();

private:
	bool ReadHeader(StdStrBuf Data);
	bool Decompress(StdBuf *pData);
};

// Loads references (mini-HTTP-client)
class C4Network2RefClient : public C4Network2HTTPClient
{
	C4GameVersion MasterVersion;
	StdCopyStrBuf MessageOfTheDay, MessageOfTheDayHyperlink;
	StdCopyStrBuf LeagueServerRedirect;
	bool fVerSet;

protected:
	virtual int32_t GetDefaultPort() { return C4NetStdPortRefServer; }

public:
	C4Network2RefClient() : fVerSet(false), C4Network2HTTPClient() {}

	bool QueryReferences();
	bool GetReferences(C4Network2Reference ** &rpReferences, int32_t &rRefCount);
	const char *GetMessageOfTheDay() const { return MessageOfTheDay.getData(); }
	const char *GetMessageOfTheDayHyperlink() const { return MessageOfTheDayHyperlink.getData(); }
	const char *GetLeagueServerRedirect() const { return LeagueServerRedirect.getLength() ? LeagueServerRedirect.getData() : nullptr; }
};
