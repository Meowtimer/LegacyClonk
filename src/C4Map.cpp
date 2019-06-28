/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Create map from dynamic landscape data in scenario */

#include <C4Include.h>
#include <C4Map.h>

#ifndef BIG_C4INCLUDE
#include <C4Random.h>
#include <C4Texture.h>
#include <C4Group.h>
#endif

#include <Bitmap256.h>

C4MapCreator::C4MapCreator()
{
	Reset();
}

void C4MapCreator::Reset()
{
	MapIFT = 128;
	MapBuf = nullptr;
	Exclusive = -1;
}

void C4MapCreator::SetPix(int32_t x, int32_t y, uint8_t col)
{
	// Safety
	if (!Inside<int32_t>(x, 0, MapWdt - 1) || !Inside<int32_t>(y, 0, MapHgt - 1)) return;
	// Exclusive
	if (Exclusive > -1) if (GetPix(x, y) != Exclusive) return;
	// Set pix
	MapBuf->SetPix(x, y, col);
}

void C4MapCreator::DrawLayer(int32_t x, int32_t y, int32_t size, uint8_t col)
{
	int32_t cnt, cnt2;
	for (cnt = 0; cnt < size; cnt++)
	{
		x += Random(9) - 4; y += Random(3) - 1;
		for (cnt2 = Random(3); cnt2 < 5; cnt2++)
		{
			SetPix(x + cnt2, y, col); SetPix(x + cnt2 + 1, y + 1, col);
		}
	}
}

uint8_t C4MapCreator::GetPix(int32_t x, int32_t y)
{
	// Safety
	if (!Inside<int32_t>(x, 0, MapWdt - 1) || !Inside<int32_t>(y, 0, MapHgt - 1)) return 0;
	// Get pix
	return MapBuf->GetPix(x, y);
}

void C4MapCreator::Create(CSurface8 *sfcMap,
	C4SLandscape &rLScape, C4TextureMap &rTexMap,
	bool fLayers, int32_t iPlayerNum)
{
	double fullperiod = 20.0 * M_PI;
	uint8_t ccol;
	int32_t cx, cy;

	// Safeties
	if (!sfcMap) return;
	iPlayerNum = BoundBy<int32_t>(iPlayerNum, 1, C4S_MaxPlayer);

	// Set creator variables
	MapBuf = sfcMap;
	MapWdt = MapBuf->Wdt; MapHgt = MapBuf->Hgt;

	// Reset map (0 is sky)
	MapBuf->ClearBox8Only(0, 0, MapBuf->Wdt, MapBuf->Hgt);

	// Surface
	ccol = rTexMap.GetIndexMatTex(rLScape.Material, "Smooth") + MapIFT;
	float amplitude = (float)rLScape.Amplitude.Evaluate();
	float phase = (float)rLScape.Phase.Evaluate();
	float period = (float)rLScape.Period.Evaluate();
	if (rLScape.MapPlayerExtend) period *= (std::min)(iPlayerNum, C4S_MaxMapPlayerExtend);
	float natural = (float)rLScape.Random.Evaluate();
	int32_t level0 = (std::min)(MapWdt, MapHgt) / 2;
	int32_t maxrange = level0 * 3 / 4;
	double cy_curve, cy_natural; // -1.0 - +1.0 !

	double rnd_cy, rnd_tend; // -1.0 - +1.0 !
	rnd_cy = (double)(Random(2000 + 1) - 1000) / 1000.0;
	rnd_tend = (double)(Random(200 + 1) - 100) / 20000.0;

	for (cx = 0; cx < MapWdt; cx++)
	{
		rnd_cy += rnd_tend;
		rnd_tend += (double)(Random(100 + 1) - 50) / 10000;
		if (rnd_tend > +0.05) rnd_tend = +0.05;
		if (rnd_tend < -0.05) rnd_tend = -0.05;
		if (rnd_cy < -0.5) rnd_tend += 0.01;
		if (rnd_cy > +0.5) rnd_tend -= 0.01;

		cy_natural = rnd_cy * natural / 100.0;
		cy_curve = sin(fullperiod * period / 100.0 * (float)cx / (float)MapWdt +
			2.0 * M_PI * phase / 100.0) * amplitude / 100.0;

		cy = level0 + BoundBy((int32_t)((float)maxrange * (cy_curve + cy_natural)),
			-maxrange, +maxrange);

		SetPix(cx, cy, ccol);
	}

	// Raise bottom to surface
	for (cx = 0; cx < MapWdt; cx++)
		for (cy = MapHgt - 1; (cy >= 0) && !GetPix(cx, cy); cy--)
			SetPix(cx, cy, ccol);
	// Raise liquid level
	Exclusive = 0;
	ccol = rTexMap.GetIndexMatTex(rLScape.Liquid, "Smooth");
	int32_t wtr_level = rLScape.LiquidLevel.Evaluate();
	for (cx = 0; cx < MapWdt; cx++)
		for (cy = MapHgt * (100 - wtr_level) / 100; cy < MapHgt; cy++)
			SetPix(cx, cy, ccol);
	Exclusive = -1;

	// Layers
	if (fLayers)
	{
		// Base material
		Exclusive = rTexMap.GetIndexMatTex(rLScape.Material, "Smooth") + MapIFT;

		int32_t cnt, clayer, layer_num, sptx, spty;

		// Process layer name list
		for (clayer = 0; clayer < C4MaxNameList; clayer++)
			if (rLScape.Layers.Name[clayer][0])
			{
				// Draw layers
				ccol = rTexMap.GetIndexMatTex(rLScape.Layers.Name[clayer], "Rough") + MapIFT;
				layer_num = rLScape.Layers.Count[clayer];
				layer_num = layer_num * MapWdt * MapHgt / 15000;
				for (cnt = 0; cnt < layer_num; cnt++)
				{
					// Place layer
					sptx = Random(MapWdt);
					for (spty = 0; (spty < MapHgt) && (GetPix(sptx, spty) != Exclusive); spty++);
					spty += 5 + Random((MapHgt - spty) - 10);
					auto r1 = Random(15);
					DrawLayer(sptx, spty, r1, ccol);
				}
			}

		Exclusive = -1;
	}
}
