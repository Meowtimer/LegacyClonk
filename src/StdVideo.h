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

/* Some functions to help with loading and saving AVI files using Video for Windows */

#pragma once

#ifdef _WIN32

#include <mmsystem.h>
#include <vfw.h>
#include <io.h>
#include "StdBuf.h"

BOOL AVIOpenOutput(const char *szFilename,
	PAVIFILE *ppAviFile,
	PAVISTREAM *ppAviStream,
	int iWidth, int iHeight);

BOOL AVICloseOutput(PAVIFILE *ppAviFile,
	PAVISTREAM *ppAviStream);

BOOL AVIPutFrame(PAVISTREAM pAviStream,
	long lFrame,
	void *lpInfo, long lInfoSize,
	void *lpData, long lDataSize);

// AVI file reading class
class CStdAVIFile
{
private:
	// file data
	StdStrBuf sFilename;
	PAVIFILE pAVIFile;

	// video data
	AVISTREAMINFO StreamInfo;
	PAVISTREAM pStream;
	PGETFRAME pGetFrame;

	// video processing helpers
	BITMAPINFO *pbmi;
	HBITMAP hBitmap;
	HDRAWDIB hDD;
	HDC hDC;
	BYTE *pFrameData;
	HWND hWnd;

	// audio data
	PAVISTREAM pAudioStream;
	BYTE *pAudioData;
	LONG iAudioDataLength, iAudioBufferLength;
	WAVEFORMAT *pAudioInfo;
	LONG iAudioInfoLength;

	// frame data
	int32_t iFinalFrame, iWdt, iHgt;
	time_t iTimePerFrame; // [ms/frame]

public:
	CStdAVIFile();
	~CStdAVIFile();

	void Clear();
	bool OpenFile(const char *szFilename, HWND hWnd, int32_t iOutBitDepth);

	int32_t GetWdt() const { return iWdt; }
	int32_t GetHgt() const { return iHgt; }

	// get desired frame from time offset to video start - return false if past video length
	bool GetFrameByTime(time_t iTime, int32_t *piFrame);

	// dump RGBa-data for specified frame
	bool GrabFrame(int32_t iFrame, class CSurface *sfc) const;

	// getting audio data
	bool OpenAudioStream();
	BYTE *GetAudioStreamData(size_t *piStreamLength);
	void CloseAudioStream();
};

#endif // _WIN32
