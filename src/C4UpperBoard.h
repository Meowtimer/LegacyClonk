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

#include <C4Facet.h>

class C4UpperBoard
{
	friend class C4GraphicsSystem;

public:
	C4UpperBoard();
	~C4UpperBoard();
	void Default();
	void Clear();
	void Init(C4Facet &cgo);
	void Execute();

protected:
	void Draw(C4Facet &cgo);
	C4Facet Output;
	char cTimeString[64];
	char cTimeString2[64];
	int TextWidth;
	int TextYPosition;
};
