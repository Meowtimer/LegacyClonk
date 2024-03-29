/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2003, Sven2
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

/* freetype support by JanL and Guenther */
// text drawing facility for CStdDDraw

#include <Standard.h>
#include <StdBuf.h>
#include <StdDDraw2.h>
#include <StdSurface2.h>
#include <StdMarkup.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <tchar.h>
#include <stdio.h>
#else
#define _T(x) x
#endif // _WIN32

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif // HAVE_FREETYPE

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV

/* Initialization */

#ifdef HAVE_FREETYPE
class CStdVectorFont
{
	FT_Library library;
	FT_Face face;

public:
	CStdVectorFont(const char *filepathname)
	{
		// Initialize Freetype
		if (FT_Init_FreeType(&library))
			throw std::runtime_error("Cannot init Freetype");
		// Load the font
		FT_Error e;
		if (e = FT_New_Face(library, filepathname, 0, &face))
			throw std::runtime_error(std::string("Cannot load ") + filepathname + ": " + FormatString("%d", e).getData());
	}

	CStdVectorFont(const StdBuf &Data)
	{
		// Initialize Freetype
		if (FT_Init_FreeType(&library))
			throw std::runtime_error("Cannot init Freetype");
		// Load the font
		FT_Error e;
		if (e = FT_New_Memory_Face(library, static_cast<const FT_Byte *>(Data.getData()), Data.getSize(), 0, &face))
			throw std::runtime_error(std::string("Cannot load font: ") + FormatString("%d", e).getData());
	}

	~CStdVectorFont()
	{
		FT_Done_Face(face);
		FT_Done_FreeType(library);
	}

	operator FT_Face() { return face; }
	FT_Face operator->() { return face; }
};

CStdVectorFont *CStdFont::CreateFont(const char *szFaceName)
{
	return new CStdVectorFont(szFaceName);
}

CStdVectorFont *CStdFont::CreateFont(const StdBuf &Data)
{
	return new CStdVectorFont(Data);
}

void CStdFont::DestroyFont(CStdVectorFont *pFont)
{
	delete pFont;
}

#elif defined(_WIN32)

class CStdVectorFont
{
private:
	StdStrBuf sFontName;

public:
	CStdVectorFont(const char *name)
	{
		sFontName.Copy(name);
	}

	const char *GetFontName() { return sFontName.getData(); }
};

CStdVectorFont *CStdFont::CreateFont(const char *szFaceName)
{
	return new CStdVectorFont(szFaceName);
}

void CStdFont::DestroyFont(CStdVectorFont *pFont)
{
	delete pFont;
}

#else

CStdVectorFont *CStdFont::CreateFont(const StdBuf &Data)
{
	return 0;
}

CStdVectorFont *CStdFont::CreateFont(const char *szFaceName)
{
	return 0;
}

void CStdFont::DestroyFont(CStdVectorFont *pFont) {}

#endif

CStdFont::CStdFont()
{
	// set default values
	psfcFontData = nullptr;
	sfcCurrent = nullptr;
	iNumFontSfcs = 0;
	iSfcSizes = 64;
	dwDefFontHeight = iLineHgt = 10;
	iFontZoom = 1; // default: no internal font zooming - likely no antialiasing either...
	iHSpace = -1;
	iGfxLineHgt = iLineHgt + 1;
	dwWeight = FW_NORMAL;
	fDoShadow = false;
	fUTF8 = false;
	// font not yet initialized
	*szFontName = 0;
	id = 0;
	pCustomImages = nullptr;
	fPrerenderedFont = false;
#if defined(_WIN32) && !defined(HAVE_FREETYPE)
	hDC = nullptr;
	hbmBitmap = nullptr;
	hFont = nullptr;
#elif defined(HAVE_FREETYPE)
	pVectorFont = nullptr;
#endif
}

bool CStdFont::AddSurface()
{
	// add new surface as render target; copy old ones
	CSurface **pNewSfcs = new CSurface *[iNumFontSfcs + 1];
	if (iNumFontSfcs) memcpy(pNewSfcs, psfcFontData, iNumFontSfcs * sizeof(CSurface *));
	delete[] psfcFontData;
	psfcFontData = pNewSfcs;
	CSurface *sfcNew = psfcFontData[iNumFontSfcs] = new CSurface();
	++iNumFontSfcs;
	if (iSfcSizes) if (!sfcNew->Create(iSfcSizes, iSfcSizes)) return false;
	sfcCurrent = sfcNew;
	iCurrentSfcX = iCurrentSfcY = 0;
	return true;
}

bool CStdFont::CheckRenderedCharSpace(uint32_t iCharWdt, uint32_t iCharHgt)
{
	// need to do a line break?
	if (iCurrentSfcX + iCharWdt >= (uint32_t)iSfcSizes) if (iCurrentSfcX)
	{
		iCurrentSfcX = 0;
		iCurrentSfcY += iCharHgt;
		if (iCurrentSfcY + iCharHgt >= (uint32_t)iSfcSizes)
		{
			// surface is full: Next one
			if (!AddSurface()) return false;
		}
	}
	// OK draw it there
	return true;
}

bool CStdFont::AddRenderedChar(uint32_t dwChar, CFacet *pfctTarget)
{
#if defined(_WIN32) && !defined(HAVE_FREETYPE)

	// Win32-API character rendering
	// safety
	if (fPrerenderedFont || !sfcCurrent) return false;
	bool fUnicode = (dwChar >= 256);
	char str[2] = _T("x");
	wchar_t wstr[2] = L"x";
	SIZE size;
	if (fUnicode)
	{
		wstr[0] = dwChar;
		GetTextExtentPoint32W(hDC, wstr, 1, &size);
	}
	else
	{
		// set character
		str[0] = dwChar;
		// get size
		GetTextExtentPoint32(hDC, str, 1, &size);
	}
	// keep text shadow in mind
	if (fDoShadow) { ++size.cx; ++size.cy; }
	// adjust line height to max character height
	if (!fUnicode) iLineHgt = std::max<int>(iLineHgt, size.cy + 1);
	// print character on empty surface
	std::fill_n(pBitmapBits, iBitmapSize * iBitmapSize, 0);
	if (fUnicode)
		ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, nullptr, wstr, 1, nullptr);
	else
		ExtTextOut(hDC, 0, 0, ETO_OPAQUE, nullptr, str, 1, nullptr);
	// must not overflow surfaces: do some size bounds
	size.cx = std::min<int>(size.cx, std::min<int>(iSfcSizes, iBitmapSize));
	size.cy = std::min<int>(size.cy, std::min<int>(iSfcSizes, iBitmapSize));
	// need to do a line break or new surface?
	if (!CheckRenderedCharSpace(size.cx, size.cy)) return false;
	// transfer bitmap data into alpha channel of surface
	if (!sfcCurrent->Lock()) return false;
	for (int y = 0; y < size.cy; ++y) for (int x = 0; x < size.cx; ++x)
	{
		// get value; determine shadow value by pos moved 1px to upper left
		uint8_t bAlpha = 255 - (uint8_t)(pBitmapBits[iBitmapSize * y + x] & 0xff);
		uint8_t bAlphaShadow;
		if (x && y && fDoShadow)
			bAlphaShadow = 255 - (uint8_t)((pBitmapBits[iBitmapSize * (y - 1) + x - 1] & 0xff) * 1 / 1);
		else
			bAlphaShadow = 255;
		// calc pixel value: white char on black shadow (if shadow is desired)
		uint32_t dwPixVal = bAlphaShadow << 24;
		BltAlpha(dwPixVal, bAlpha << 24 | 0xffffff);
		sfcCurrent->SetPixDw(iCurrentSfcX + x, iCurrentSfcY + y, dwPixVal);
	}
	sfcCurrent->Unlock();
	// set texture coordinates
	pfctTarget->Set(sfcCurrent, iCurrentSfcX, iCurrentSfcY, size.cx, size.cy);

#elif defined(HAVE_FREETYPE)

	// Freetype character rendering
	FT_Set_Pixel_Sizes(*pVectorFont, dwDefFontHeight, dwDefFontHeight);
	int32_t iBoldness = dwWeight - 400; // zero is normal; 300 is bold
	if (iBoldness)
	{
		iBoldness = (1 << 16) + (iBoldness << 16) / 400;
		FT_Matrix mat;
		mat.xx = iBoldness; mat.xy = mat.yx = 0; mat.yy = 1 << 16;
		// .*(100 + iBoldness/3)/100
		FT_Set_Transform(*pVectorFont, &mat, nullptr);
	}
	else
	{
		FT_Set_Transform(*pVectorFont, nullptr, nullptr);
	}
	// Render
	if (FT_Load_Char(*pVectorFont, dwChar, FT_LOAD_RENDER | FT_LOAD_NO_HINTING))
	{
		// although the character was not drawn, assume it's not in the font and won't be needed
		// so return success here
		return true;
	}
	// Make a shortcut to the glyph
	FT_GlyphSlot slot = (*pVectorFont)->glyph;
	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
	{
		// although the character was drawn in a strange way, assume it's not in the font and won't be needed
		// so return success here
		return true;
	}
	// linebreak/ new surface check
	int width = std::max<int>(slot->advance.x / 64, (std::max)(slot->bitmap_left, 0) + slot->bitmap.width) + fDoShadow;
	if (!CheckRenderedCharSpace(width, iGfxLineHgt)) return false;
	// offset from the top
	int at_y = iCurrentSfcY + dwDefFontHeight * (*pVectorFont)->ascender / (*pVectorFont)->units_per_EM - slot->bitmap_top;
	int at_x = iCurrentSfcX + (std::max)(slot->bitmap_left, 0);
	// Copy to the surface
	if (!sfcCurrent->Lock()) return false;
	for (int y = 0; y < slot->bitmap.rows + fDoShadow; ++y)
	{
		for (int x = 0; x < slot->bitmap.width + fDoShadow; ++x)
		{
			unsigned char bAlpha, bAlphaShadow;
			if (x < slot->bitmap.width && y < slot->bitmap.rows)
				bAlpha = 255 - (unsigned char)(slot->bitmap.buffer[slot->bitmap.width * y + x]);
			else
				bAlpha = 255;
			// Make a shadow from the upper-left pixel, and blur with the eight neighbors
			uint32_t dwPixVal = 0u;
			bAlphaShadow = 255;
			if ((x || y) && fDoShadow)
			{
				int iShadow = 0;
				if (x < slot->bitmap.width && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - 0) + slot->bitmap.width * (y - 0)];
				if (x > 1 && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - 2) + slot->bitmap.width * (y - 0)];
				if (x > 0 && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - 1) + slot->bitmap.width * (y - 0)];
				if (x < slot->bitmap.width && y > 1) iShadow += slot->bitmap.buffer[(x - 0) + slot->bitmap.width * (y - 2)];
				if (x > 1 && y > 1) iShadow += slot->bitmap.buffer[(x - 2) + slot->bitmap.width * (y - 2)];
				if (x > 0 && y > 1) iShadow += slot->bitmap.buffer[(x - 1) + slot->bitmap.width * (y - 2)];
				if (x < slot->bitmap.width && y > 0) iShadow += slot->bitmap.buffer[(x - 0) + slot->bitmap.width * (y - 1)];
				if (x > 1 && y > 0) iShadow += slot->bitmap.buffer[(x - 2) + slot->bitmap.width * (y - 1)];
				if (x > 0 && y > 0) iShadow += slot->bitmap.buffer[(x - 1) + slot->bitmap.width * (y - 1)] * 8;
				bAlphaShadow -= iShadow / 16;
				// because blitting on a black pixel reduces luminosity as compared to shadowless font,
				// assume luminosity as if blitting shadowless font on a 50% gray background
				unsigned char cBack = (255 - bAlpha);
				dwPixVal = RGB(cBack / 2, cBack / 2, cBack / 2);
			}
			dwPixVal += bAlphaShadow << 24;
			BltAlpha(dwPixVal, bAlpha << 24 | 0xffffff);
			sfcCurrent->SetPixDw(at_x + x, at_y + y, dwPixVal);
		}
	}
	sfcCurrent->Unlock();
	// Save the position of the glyph for the rendering code
	pfctTarget->Set(sfcCurrent, iCurrentSfcX, iCurrentSfcY, width, iGfxLineHgt);

#endif // end of freetype rendering

	// advance texture position
	iCurrentSfcX += pfctTarget->Wdt;
	return true;
}

uint32_t CStdFont::GetNextUTF8Character(const char **pszString)
{
	// assume the current character is UTF8 already (i.e., highest bit set)
	const char *szString = *pszString;
	unsigned char c = *szString++;
	uint32_t dwResult = '?';
	assert(c > 127);
	if (c > 191 && c < 224)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return '?'; }
		dwResult = (int(c & 31) << 6) | (c2 & 63); // two char code
	}
	else if (c >= 224 && c <= 239)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return '?'; }
		unsigned char c3 = *szString++;
		if ((c3 & 192) != 128) { *pszString = szString; return '?'; }
		dwResult = (int(c & 15) << 12) | (int(c2 & 63) << 6) | int(c3 & 63); // three char code
	}
	else if (c >= 240 && c <= 247)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return '?'; }
		unsigned char c3 = *szString++;
		if ((c3 & 192) != 128) { *pszString = szString; return '?'; }
		unsigned char c4 = *szString++;
		if ((c4 & 192) != 128) { *pszString = szString; return '?'; }
		dwResult = (int(c & 7) << 18) | (int(c2 & 63) << 12) | (int(c3 & 63) << 6) | int(c4 & 63); // four char code
	}
	*pszString = szString;
	return dwResult;
}

CFacet &CStdFont::GetUnicodeCharacterFacet(uint32_t c)
{
	// find/add facet in map
	CFacet &rFacet = fctUnicodeMap[c];
	// create character on the fly if necessary and possible
	if (!rFacet.Surface && !fPrerenderedFont) AddRenderedChar(c, &rFacet);
	// rendering might have failed, in which case rFacet remains empty. Should be OK; char won't be printed then
	return rFacet;
}

void CStdFont::Init(CStdVectorFont &VectorFont, uint32_t dwHeight, uint32_t dwFontWeight, const char *szCharset, bool fDoShadow)
{
	// clear previous
	Clear();
	// set values
	iHSpace = fDoShadow ? -1 : 0; // horizontal shadow
	dwWeight = dwFontWeight;
	this->fDoShadow = fDoShadow;
	if (SEqual(szCharset, "UTF-8")) fUTF8 = true;
	// determine needed texture size
	if (dwHeight * iFontZoom > 40)
		iSfcSizes = 512;
	else if (dwDefFontHeight * iFontZoom > 20)
		iSfcSizes = 256;
	else
		iSfcSizes = 128;
	dwDefFontHeight = dwHeight;
	// create surface
	if (!AddSurface())
	{
		Clear();
		throw std::runtime_error(std::string("Cannot create surface (") + szFontName + ")");
	}

#if defined(_WIN32) && !defined(HAVE_FREETYPE)

	// drawing using WinGDI
	iLineHgt = dwHeight;
	iGfxLineHgt = iLineHgt + fDoShadow; // vertical shadow

	// prepare to create an offscreen bitmap to render into
	iBitmapSize = DWordAligned(dwDefFontHeight * iFontZoom * 5);
	BITMAPINFO bmi; bmi.bmiHeader = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = iBitmapSize;
	bmi.bmiHeader.biHeight = -iBitmapSize;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;

	// create a rendering DC and a bitmap for the font
	hDC = CreateCompatibleDC(nullptr);
	if (!hDC) { Clear(); throw std::runtime_error(std::string("Cannot create DC (") + szFontName + ")"); }
	hbmBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS,
		(VOID **)&pBitmapBits, nullptr, 0);
	if (!hbmBitmap) { Clear(); throw std::runtime_error(std::string("Cannot create DIBSection (") + szFontName + ")"); }
	char bCharset = GetCharsetCode(szCharset);
	// create a font. try ClearType first...
	const char *szFontName = VectorFont.GetFontName();
	const char *szFontName2;
	if (szFontName && *szFontName) szFontName2 = szFontName; else szFontName2 = "Comic Sans MS";
	int iFontHeight = dwDefFontHeight * GetDeviceCaps(hDC, LOGPIXELSY) * iFontZoom / 72;
	hFont = ::CreateFont(iFontHeight, 0, 0, 0, dwFontWeight, FALSE,
		FALSE, FALSE, bCharset, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, 5,
		VARIABLE_PITCH, szFontName2);

	// ClearType failed: try antialiased (not guaranteed)
	if (!hFont) hFont = ::CreateFont(iFontHeight, 0, 0, 0, dwFontWeight, FALSE,
		FALSE, FALSE, bCharset, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		VARIABLE_PITCH, szFontName2);

	if (!hFont)
	{
		Clear();
		throw std::runtime_error(std::string("Cannot create Font (") + szFontName + ")");
	}
	SelectObject(hDC, hbmBitmap);
	SelectObject(hDC, hFont);

	// set text properties
	SetTextColor(hDC, RGB(255, 255, 255));
	SetBkColor(hDC, 0x00000000);
	SetTextAlign(hDC, TA_TOP);

	// line height adjusted when characters are created
	iLineHgt = 0;

#elif defined(HAVE_FREETYPE)

	// Store vector font - assumed to be held externally!
	pVectorFont = &VectorFont;
	// Get size
	// FIXME: use bbox or dynamically determined line heights here
	iLineHgt = (VectorFont->ascender - VectorFont->descender) * dwHeight / VectorFont->units_per_EM;
	iGfxLineHgt = iLineHgt + fDoShadow; // vertical shadow

#else

	throw std::runtime_error("You have a engine without Truetype support.");

#endif // HAVE_FREETYPE

	// loop through all ANSI/ASCII printable characters and prepare them
	// in case of UTF8, unicode characters will be created on the fly and extended ASCII characters (128-255) are not needed
	// now render all characters!

#if defined(HAVE_ICONV)
	// Initialize iconv
	struct iconv_t_wrapper
	{
		iconv_t i;
		iconv_t_wrapper(const char *to, const char *from)
		{
			i = iconv_open(to, from);
			if (i == iconv_t(-1))
				throw std::runtime_error(std::string("Cannot open iconv (") + to + ", " + from + ")");
		}
		~iconv_t_wrapper() { iconv_close(i); }
		operator iconv_t() { return i; }
	};
#ifdef __BIG_ENDIAN__
	iconv_t_wrapper iconv_handle("UCS-4BE", GetCharsetCodeName(szCharset));
#else
	iconv_t_wrapper iconv_handle("UCS-4LE", GetCharsetCodeName(szCharset));
#endif
#elif defined(_WIN32)
	int32_t iCodePage = GetCharsetCodePage(szCharset);
#endif
	int cMax = fUTF8 ? 127 : 255;
	for (int c = ' '; c <= cMax; ++c)
	{
		uint32_t dwChar = c;
#if defined(HAVE_ICONV)
		// convert from whatever legacy encoding in use to unicode
		if (!fUTF8)
		{
			// Convert to unicode
			char chr = dwChar;
			char *in = &chr;
			char *out = reinterpret_cast<char *>(&dwChar);
			size_t insize = 1;
			size_t outsize = 4;
			iconv(iconv_handle, const_cast<ICONV_CONST char * *>(&in), &insize, &out, &outsize);
		}
#elif defined(_WIN32)
		// convert using Win32 API
		if (!fUTF8 && c >= 128)
		{
			char cc[2] = { c, '\0' };
			wchar_t outbuf[4];
			if (MultiByteToWideChar(iCodePage, 0, cc, -1, outbuf, 4)) // 2do: Convert using proper codepage
			{
				// now convert from UTF-16 to UCS-4
				if (((outbuf[0] & 0xfc00) == 0xd800) && ((outbuf[1] & 0xfc00) == 0xdc00))
				{
					dwChar = 0x10000 + (((outbuf[0] & 0x3ff) << 10) | (outbuf[1] & 0x3ff));
				}
				else
					dwChar = outbuf[0];
			}
			else
			{
				// conversion error. Shouldn't be fatal; just pretend it were a Unicode character
			}
		}
#else
		// no conversion available? Just break for non-iso8859-1.
#endif // defined HAVE_ICONV
		if (!AddRenderedChar(dwChar, &(fctAsciiTexCoords[c - ' '])))
		{
			Clear();
			throw std::runtime_error(std::string("Cannot render characters for Font (") + szFontName + ")");
		}
	}
	// adjust line height
	iLineHgt /= iFontZoom;

	fPrerenderedFont = false;
	if (0) for (int i = 0; i < iNumFontSfcs; ++i)
	{
		StdStrBuf pngfilename = FormatString("%s%i%s_%d.png", szFontName, dwHeight, fDoShadow ? "_shadow" : "", i);
		psfcFontData[i]->SavePNG(pngfilename.getData(), true, false, false);
	}
}

const uint32_t FontDelimeterColor        = 0xff0000,
               FontDelimiterColorLB      = 0x00ff00,
               FontDelimeterColorIndent1 = 0xffff00,
               FontDelimeterColorIndent2 = 0xff00ff;

// perform color matching in 16 bit
inline bool ColorMatch(uint32_t dwClr1, uint32_t dwClr2)
{
	return ClrDw2W(dwClr1) == ClrDw2W(dwClr2);
}

void CStdFont::Init(const char *szFontName, CSurface *psfcFontSfc, int iIndent)
{
	// clear previous
	Clear();
	// grab surface
	iSfcSizes = 0;
	if (!AddSurface()) { Clear(); throw std::runtime_error(std::string("Error creating surface for ") + szFontName); }
	sfcCurrent->MoveFrom(psfcFontSfc);
	// extract character positions from image data
	if (!sfcCurrent->Hgt || !sfcCurrent->Lock())
	{
		Clear();
		throw std::runtime_error(std::string("Error loading ") + szFontName);
	}
	// get line height
	iGfxLineHgt = 1;
	while (iGfxLineHgt < sfcCurrent->Hgt)
	{
		uint32_t dwPix = sfcCurrent->GetPixDw(0, iGfxLineHgt, false);
		if (ColorMatch(dwPix, FontDelimeterColor) || ColorMatch(dwPix, FontDelimiterColorLB) ||
			ColorMatch(dwPix, FontDelimeterColorIndent1) || ColorMatch(dwPix, FontDelimeterColorIndent2))
			break;
		++iGfxLineHgt;
	}
	// set font height and width indent
	dwDefFontHeight = iLineHgt = iGfxLineHgt - iIndent;
	iHSpace = -iIndent;
	// determine character sizes
	int iX = 0, iY = 0;
	for (int c = ' '; c < 256; ++c)
	{
		// save character pos
		fctAsciiTexCoords[c - ' '].X = iX; // left
		fctAsciiTexCoords[c - ' '].Y = iY; // top
		bool IsLB = false;
		// get horizontal extent
		while (iX < sfcCurrent->Wdt)
		{
			uint32_t dwPix = sfcCurrent->GetPixDw(iX, iY, false);
			if (ColorMatch(dwPix, FontDelimeterColor) || ColorMatch(dwPix, FontDelimeterColorIndent1) || ColorMatch(dwPix, FontDelimeterColorIndent2))
				break;
			if (ColorMatch(dwPix, FontDelimiterColorLB)) { IsLB = true; break; }
			++iX;
		}
		// remove vertical line
		if (iX < sfcCurrent->Wdt)
			for (int y = 0; y < iGfxLineHgt; ++y)
				sfcCurrent->SetPixDw(iX, iY + y, 0xffffffff);
		// save char size
		fctAsciiTexCoords[c - ' '].Wdt = iX - fctAsciiTexCoords[c - ' '].X;
		fctAsciiTexCoords[c - ' '].Hgt = iGfxLineHgt;
		// next line?
		if (++iX >= sfcCurrent->Wdt || IsLB)
		{
			iY += iGfxLineHgt;
			iX = 0;
			// remove horizontal line
			if (iY < sfcCurrent->Hgt)
				for (int x = 0; x < sfcCurrent->Wdt; ++x)
					sfcCurrent->SetPixDw(x, iY, 0xffffffff);
			// skip empty line
			++iY;
			// end reached?
			if (iY + iGfxLineHgt > sfcCurrent->Hgt)
			{
				// all filled
				break;
			}
		}
	}
	// release texture data
	sfcCurrent->Unlock();
	// adjust line height
	iLineHgt /= iFontZoom;
	// set name
	SCopy(szFontName, this->szFontName);
	// mark prerendered
	fPrerenderedFont = true;
}

void CStdFont::Clear()
{
#if defined(_WIN32) && !defined(HAVE_FREETYPE)
	// clear Win32API font stuff
	if (hbmBitmap) { DeleteObject(hbmBitmap); hbmBitmap = nullptr; }
	if (hDC) { DeleteDC(hDC); hDC = nullptr; }
	if (hFont) { DeleteObject(hFont); hDC = nullptr; }
#elif defined(HAVE_FREETYPE)
	pVectorFont = nullptr;
#endif
	// clear font sfcs
	if (psfcFontData)
	{
		while (iNumFontSfcs--) delete psfcFontData[iNumFontSfcs];
		delete[] psfcFontData;
		psfcFontData = nullptr;
	}
	sfcCurrent = nullptr;
	iNumFontSfcs = 0;
	for (int c = ' '; c < 256; ++c) fctAsciiTexCoords[c - ' '].Clear();
	fctUnicodeMap.clear();
	// set default values
	dwDefFontHeight = iLineHgt = 10;
	iFontZoom = 1; // default: no internal font zooming - likely no antialiasing either...
	iHSpace = -1;
	iGfxLineHgt = iLineHgt + 1;
	dwWeight = FW_NORMAL;
	fDoShadow = false;
	fPrerenderedFont = false;
	fUTF8 = false;
	// font not yet initialized
	*szFontName = 0;
	id = 0;
}

/* Text size measurement */

bool CStdFont::GetTextExtent(const char *szText, int32_t &rsx, int32_t &rsy, bool fCheckMarkup)
{
	// safety
	if (!szText) return false;
	// keep track of each row's size
	int iRowWdt = 0, iWdt = 0, iHgt = iLineHgt;
	// ignore any markup
	CMarkup MarkupChecker(false);
	// go through all text
	while (*szText)
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szText);
		// get current char
		uint32_t c = GetNextCharacter(&szText);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// line break?
		if (c == _T('\n') || (fCheckMarkup && c == _T('|'))) { iRowWdt = 0; iHgt += iLineHgt; continue; }
		// ignore system characters
		if (c < _T(' ')) continue;
		// image?
		int iImgLgt;
		if (fCheckMarkup && c == '{' && szText[0] == '{' && szText[1] != '{' && (iImgLgt = SCharPos('}', szText + 1)) > 0 && szText[iImgLgt + 2] == '}')
		{
			char imgbuf[101];
			SCopy(szText + 1, imgbuf, (std::min)(iImgLgt, 100));
			CFacet fct;
			// image renderer initialized?
			if (pCustomImages)
				// try to get an image then
				pCustomImages->GetFontImage(imgbuf, fct);
			if (fct.Hgt)
			{
				// image found: adjust aspect by font height and calc appropriate width
				iRowWdt += (fct.Wdt * iGfxLineHgt) / fct.Hgt;
			}
			else
			{
				// image renderer not hooked or ID not found, or surface not present: just ignore it
				// printing it out wouldn't look better...
			}
			// skip image tag
			szText += iImgLgt + 3;
		}
		else
		{
			// regular char
			// look up character width in texture coordinates table
			iRowWdt += GetCharacterFacet(c).Wdt / iFontZoom;
		}
		// apply horizontal indent for all but last char
		if (*szText) iRowWdt += iHSpace;
		// adjust max row size
		if (iRowWdt > iWdt) iWdt = iRowWdt;
	}
	// store output
	rsx = iWdt; rsy = iHgt;
	// done, success
	return true;
}

int CStdFont::BreakMessage(const char *szMsg, int iWdt, StdStrBuf *pOut, bool fCheckMarkup, float fZoom)
{
	// safety
	if (!szMsg || !pOut) return 0;
	pOut->Clear();
	uint32_t c;
	const char *szPos = szMsg, // current parse position in the text
		*szLastBreakPos = szMsg, // points to the char after at (whitespace) or after ('-') which text can be broken
		*szLastEmergenyBreakPos, // same, but at last char in case no suitable linebreak could be found
		*szLastPos;              // last position until which buffer has been transferred to output
	int iLastBreakOutLen, iLastEmergencyBreakOutLen; // size of output string at break positions
	int iX = 0, // current text width at parse pos
		iXBreak = 0, // text width as it was at last break pos
		iXEmergencyBreak, // same, but at last char in case no suitable linebreak could be found
		iHgt = iLineHgt; // total height of output text
	bool fIsFirstLineChar = true;
	// ignore any markup
	CMarkup MarkupChecker(false);
	// go through all text
	while (*(szLastPos = szPos))
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szPos);
		// get current char
		c = GetNextCharacter(&szPos);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// manual break?
		int iCharWdt = 0;
		if (c != '\n' && (!fCheckMarkup || c != '|'))
		{
			// image?
			int iImgLgt;
			if (fCheckMarkup && c == '{' && szPos[0] == '{' && szPos[1] != '{' && (iImgLgt = SCharPos('}', szPos + 1)) > 0 && szPos[iImgLgt + 2] == '}')
			{
				char imgbuf[101];
				SCopy(szPos + 1, imgbuf, (std::min)(iImgLgt, 100));
				CFacet fct;
				// image renderer initialized?
				if (pCustomImages)
					// try to get an image then
					pCustomImages->GetFontImage(imgbuf, fct);
				if (fct.Hgt)
				{
					// image found: adjust aspect by font height and calc appropriate width
					iCharWdt = (fct.Wdt * iGfxLineHgt) / fct.Hgt;
				}
				else
				{
					// image renderer not hooked or ID not found, or surface not present: just ignore it
					// printing it out wouldn't look better...
					iCharWdt = 0;
				}
				// skip image tag
				szPos += iImgLgt + 3;
			}
			else
			{
				// regular char
				// look up character width in texture coordinates table
				if (c >= ' ')
					iCharWdt = int(fZoom * GetCharacterFacet(c).Wdt / iFontZoom) + iHSpace;
				else
					iCharWdt = 0; // OMFG ctrl char
			}
			// add chars to output
			pOut->Append(szLastPos, szPos - szLastPos);
			// add to line; always add one char at minimum
			if ((iX += iCharWdt) <= iWdt || fIsFirstLineChar)
			{
				// check whether linebreak possibility shall be marked here
				// 2do: What about unicode-spaces?
				if (c < 256) if (isspace((unsigned char)c) || c == '-')
				{
					szLastBreakPos = szPos;
					iLastBreakOutLen = pOut->getLength();
					// space: Break directly at space if it isn't the first char here
					// first char spaces must remain, in case the output area is just one char width
					if (c != '-' && !fIsFirstLineChar) --szLastBreakPos; // because c<256, the character length can be safely assumed to be 1 here
					iXBreak = iX;
				}
				// always mark emergency break after char that fitted the line
				szLastEmergenyBreakPos = szPos;
				iXEmergencyBreak = iX;
				iLastEmergencyBreakOutLen = pOut->getLength();
				// line OK; continue filling it
				fIsFirstLineChar = false;
				continue;
			}
			// line must be broken now
			// check if a linebreak is possible directly here, because it's a space
			// only check for space and not for other breakable characters (such as '-'), because the break would happen after those characters instead of at them
			if (c < 128 && isspace((unsigned char)c))
			{
				szLastBreakPos = szPos - 1;
				iLastBreakOutLen = pOut->getLength();
				iXBreak = iX;
			}
			// if there was no linebreak, do it at emergency pos
			else if (szLastBreakPos == szMsg)
			{
				szLastBreakPos = szLastEmergenyBreakPos;
				iLastBreakOutLen = iLastEmergencyBreakOutLen;
				iXBreak = iXEmergencyBreak;
			}
			StdStrBuf tempPart;
			// insert linebreak at linebreak pos
			// was it a space? Then just overwrite space with a linebreak
			if (uint8_t(*szLastBreakPos) < 128 && isspace((unsigned char)*szLastBreakPos))
			{
				*pOut->getMPtr(iLastBreakOutLen - 1) = '\n';
				if (fCheckMarkup)
				{
					tempPart.Copy(pOut->getMPtr(iLastBreakOutLen));
					pOut->SetLength(iLastBreakOutLen);
				}
			}
			else
			{
				// otherwise, insert line break
				pOut->InsertChar('\n', iLastBreakOutLen);
				if (fCheckMarkup)
				{
					tempPart.Copy(pOut->getMPtr(iLastBreakOutLen) + 1);
					pOut->SetLength(iLastBreakOutLen + 1);
				}
			}
			if (fCheckMarkup)
			{
				CMarkup markup(false);
				const char *data = pOut->getData();
				const char *lastLine = (std::max)(data + pOut->getSize() - 3, data);
				while (lastLine > data && *lastLine != '\n') --lastLine;
				while (*lastLine)
				{
					while (*lastLine == '<' && markup.Read(&lastLine));
					if (*lastLine) ++lastLine;
				}
				(*pOut) += markup.ToMarkup() + tempPart;
			}
			// calc next line usage
			iX -= iXBreak;
		}
		else
		{
			// a static linebreak: Everything's well; this just resets the line width
			iX = 0;
			// add to output
			pOut->Append(szLastPos, szPos - szLastPos);
		}
		// forced or manual line break: set new line beginning to char after line break
		szLastBreakPos = szMsg = szPos;
		// manual line break or line width overflow: add char to next line
		iHgt += iLineHgt;
		fIsFirstLineChar = true;
	}
	// transfer final data to buffer (any missing markup)
	pOut->Append(szLastPos, szPos - szLastPos);
	// return text height
	return iHgt;
}

/* Text drawing */

void CStdFont::DrawText(CSurface *sfcDest, int iX, int iY, uint32_t dwColor, const char *szText, uint32_t dwFlags, CMarkup &Markup, float fZoom)
{
	CBltTransform bt, *pbt = nullptr;
	// set blit color
	dwColor = InvertRGBAAlpha(dwColor);
	uint32_t dwOldModClr;
	bool fWasModulated = lpDDraw->GetBlitModulation(dwOldModClr);
	if (fWasModulated) ModulateClr(dwColor, dwOldModClr);
	// get alpha fade percentage
	uint32_t dwAlphaMod = BoundBy<int>((((int)(dwColor >> 0x18) - 0x50) * 0xff) / 0xaf, 0, 255) << 0x18 | 0xffffff;
	// adjust text starting position (horizontal only)
	if (dwFlags & STDFONT_CENTERED)
	{
		// centered
		int32_t sx, sy;
		GetTextExtent(szText, sx, sy, !(dwFlags & STDFONT_NOMARKUP));
		sx = int(fZoom * sx); sy = int(fZoom * sy);
		iX -= sx / 2;
	}
	else if (dwFlags & STDFONT_RIGHTALGN)
	{
		// right-aligned
		int32_t sx, sy;
		GetTextExtent(szText, sx, sy, !(dwFlags & STDFONT_NOMARKUP));
		sx = int(fZoom * sx); sy = int(fZoom * sy);
		iX -= sx;
	}
	// apply texture zoom
	fZoom /= iFontZoom;
	// set start markup transformation
	if (!Markup.Clean()) pbt = &bt;
	// output text
	uint32_t c;
	CFacet fctFromBlt; // source facet
	while (c = GetNextCharacter(&szText))
	{
		// ignore system characters
		if (c < _T(' ')) continue;
		// apply markup
		if (c == '<' && (~dwFlags & STDFONT_NOMARKUP))
		{
			// get tag
			if (Markup.Read(&--szText))
			{
				// mark transform to be done
				// (done only if tag was found, so most normal blits don't init a trasnformation matrix)
				pbt = &bt;
				// skip the tag
				continue;
			}
			// invalid tag: render it as text
			++szText;
		}
		int w2, h2; // dst width/height
		// custom image?
		int iImgLgt;
		if (c == '{' && szText[0] == '{' && szText[1] != '{' && (iImgLgt = SCharPos('}', szText + 1)) > 0 && szText[iImgLgt + 2] == '}' && !(dwFlags & STDFONT_NOMARKUP))
		{
			fctFromBlt.Default();
			char imgbuf[101];
			SCopy(szText + 1, imgbuf, (std::min)(iImgLgt, 100));
			szText += iImgLgt + 3;
			// image renderer initialized?
			if (pCustomImages)
				// try to get an image then
				pCustomImages->GetFontImage(imgbuf, fctFromBlt);
			if (fctFromBlt.Surface && fctFromBlt.Hgt)
			{
				// image found: adjust aspect by font height and calc appropriate width
				w2 = (fctFromBlt.Wdt * iGfxLineHgt) / fctFromBlt.Hgt;
				h2 = iGfxLineHgt;
			}
			else
			{
				// image renderer not hooked or ID not found, or surface not present: just ignore it
				// printing it out wouldn't look better...
				continue;
			}
			// normal: not modulated, unless done by transform or alpha fadeout
			if ((dwColor >> 0x18) <= 0x50)
				lpDDraw->DeactivateBlitModulation();
			else
				lpDDraw->ActivateBlitModulation((dwColor & 0xff000000) | 0xffffff);
		}
		else
		{
			// regular char
			// get texture coordinates
			fctFromBlt = GetCharacterFacet(c);
			w2 = int(fctFromBlt.Wdt * fZoom); h2 = int(fctFromBlt.Hgt * fZoom);
			lpDDraw->ActivateBlitModulation(dwColor);
		}
		// do color/markup
		if (pbt)
		{
			// reset data to be transformed by markup
			uint32_t dwBlitClr = dwColor;
			bt.Set(1, 0, 0, 0, 1, 0, 0, 0, 1);
			// apply markup
			Markup.Apply(bt, dwBlitClr);
			if (dwBlitClr != dwColor) ModulateClrA(dwBlitClr, dwAlphaMod);
			lpDDraw->ActivateBlitModulation(dwBlitClr);
			// move transformation center to center of letter
			float fOffX = (float)w2 / 2 + iX;
			float fOffY = (float)h2 / 2 + iY;
			bt.mat[2] += fOffX - fOffX * bt.mat[0] - fOffY * bt.mat[1];
			bt.mat[5] += fOffY - fOffX * bt.mat[3] - fOffY * bt.mat[4];
		}
		// blit character or image
		lpDDraw->Blit(fctFromBlt.Surface, float(fctFromBlt.X), float(fctFromBlt.Y), float(fctFromBlt.Wdt), float(fctFromBlt.Hgt),
			sfcDest, iX, iY, w2, h2,
			true, pbt);
		// advance pos and skip character indent
		iX += w2 + iHSpace;
	}
	// reset blit modulation
	if (fWasModulated)
		lpDDraw->ActivateBlitModulation(dwOldModClr);
	else
		lpDDraw->DeactivateBlitModulation();
}

// The internal clonk charset is one of the windows charsets
// But to save the used one to the configuration, a string is used
// So we need to convert this string to the windows number for windows
// and RTF, and to the iconv name for iconv
const char *GetCharsetCodeName(const char *strCharset)
{
	// Match charset name to WinGDI codes
	if (SEqualNoCase(strCharset, "SHIFTJIS"))    return "CP932";
	if (SEqualNoCase(strCharset, "HANGUL"))      return "CP949";
	if (SEqualNoCase(strCharset, "JOHAB"))       return "CP1361";
	if (SEqualNoCase(strCharset, "CHINESEBIG5")) return "CP950";
	if (SEqualNoCase(strCharset, "GREEK"))       return "CP1253";
	if (SEqualNoCase(strCharset, "TURKISH"))     return "CP1254";
	if (SEqualNoCase(strCharset, "VIETNAMESE"))  return "CP1258";
	if (SEqualNoCase(strCharset, "HEBREW"))      return "CP1255";
	if (SEqualNoCase(strCharset, "ARABIC"))      return "CP1256";
	if (SEqualNoCase(strCharset, "BALTIC"))      return "CP1257";
	if (SEqualNoCase(strCharset, "RUSSIAN"))     return "CP1251";
	if (SEqualNoCase(strCharset, "THAI"))        return "CP874";
	if (SEqualNoCase(strCharset, "EASTEUROPE"))  return "CP1250";
	if (SEqualNoCase(strCharset, "UTF-8"))       return "UTF-8";
	// Default
	return "CP1252";
}

uint8_t GetCharsetCode(const char *strCharset)
{
	// Match charset name to WinGDI codes
	if (SEqualNoCase(strCharset, "SHIFTJIS"))    return 128; // SHIFTJIS_CHARSET
	if (SEqualNoCase(strCharset, "HANGUL"))      return 129; // HANGUL_CHARSET
	if (SEqualNoCase(strCharset, "JOHAB"))       return 130; // JOHAB_CHARSET
	if (SEqualNoCase(strCharset, "CHINESEBIG5")) return 136; // CHINESEBIG5_CHARSET
	if (SEqualNoCase(strCharset, "GREEK"))       return 161; // GREEK_CHARSET
	if (SEqualNoCase(strCharset, "TURKISH"))     return 162; // TURKISH_CHARSET
	if (SEqualNoCase(strCharset, "VIETNAMESE"))  return 163; // VIETNAMESE_CHARSET
	if (SEqualNoCase(strCharset, "HEBREW"))      return 177; // HEBREW_CHARSET
	if (SEqualNoCase(strCharset, "ARABIC"))      return 178; // ARABIC_CHARSET
	if (SEqualNoCase(strCharset, "BALTIC"))      return 186; // BALTIC_CHARSET
	if (SEqualNoCase(strCharset, "RUSSIAN"))     return 204; // RUSSIAN_CHARSET
	if (SEqualNoCase(strCharset, "THAI"))        return 222; // THAI_CHARSET
	if (SEqualNoCase(strCharset, "EASTEUROPE"))  return 238; // EASTEUROPE_CHARSET
	if (SEqualNoCase(strCharset, "UTF-8"))       return 0;   // ANSI_CHARSET - UTF8 needs special handling
	// Default
	return 0; // ANSI_CHARSET
}

int32_t GetCharsetCodePage(const char *strCharset)
{
	// Match charset name to WinGDI codes
	if (SEqualNoCase(strCharset, "SHIFTJIS"))    return 932;
	if (SEqualNoCase(strCharset, "HANGUL"))      return 949;
	if (SEqualNoCase(strCharset, "JOHAB"))       return 1361;
	if (SEqualNoCase(strCharset, "CHINESEBIG5")) return 950;
	if (SEqualNoCase(strCharset, "GREEK"))       return 1253;
	if (SEqualNoCase(strCharset, "TURKISH"))     return 1254;
	if (SEqualNoCase(strCharset, "VIETNAMESE"))  return 1258;
	if (SEqualNoCase(strCharset, "HEBREW"))      return 1255;
	if (SEqualNoCase(strCharset, "ARABIC"))      return 1256;
	if (SEqualNoCase(strCharset, "BALTIC"))      return 1257;
	if (SEqualNoCase(strCharset, "RUSSIAN"))     return 1251;
	if (SEqualNoCase(strCharset, "THAI"))        return 874;
	if (SEqualNoCase(strCharset, "EASTEUROPE"))  return 1250;
	if (SEqualNoCase(strCharset, "UTF-8"))       return -1; // shouldn't be called
	// Default
	return 1252;
}
