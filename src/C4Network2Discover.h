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

#include "C4NetIO.h"

const int C4NetMaxDiscover = 64;

const unsigned long C4NetDiscoveryAddress = 0xef; // 239.0.0.0

class C4Network2IODiscover : public C4NetIOSimpleUDP, private C4NetIO::CBClass
{
public:
	C4Network2IODiscover(int16_t iRefServerPort) : iRefServerPort(iRefServerPort), fEnabled(false)
	{
		C4NetIOSimpleUDP::SetCallback(this);
	}

protected:
	// callbacks (will handle everything here)
	virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

public:
	bool Init(uint16_t iPort = P_NONE);
	void SetDiscoverable(bool fnEnabled) { fEnabled = fnEnabled; }
	bool Announce();

private:
	sockaddr_in DiscoveryAddr;

	int16_t iRefServerPort;
	bool fEnabled;
};

class C4Network2IODiscoverClient : public C4NetIOSimpleUDP, private C4NetIO::CBClass
{
public:
	C4Network2IODiscoverClient() : iDiscoverCount(0)
	{
		C4NetIOSimpleUDP::SetCallback(this);
	}

protected:
	// callbacks (will handle everything here)
	virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

public:
	int getDiscoverCount() const { return iDiscoverCount; }

	void Clear() { iDiscoverCount = 0; }
	bool Init(uint16_t iPort = P_NONE);
	bool StartDiscovery();
	bool PopDiscover(C4NetIO::addr_t &Discover);

private:
	C4NetIO::addr_t DiscoveryAddr;

	int iDiscoverCount;
	C4NetIO::addr_t Discovers[C4NetMaxDiscover];
};
