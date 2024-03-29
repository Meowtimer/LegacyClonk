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

/* Main header to include all others */

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef _WIN32
#define C4_OS "win32"
#elif defined(__linux__)
#ifdef __x86_64
#define C4_OS "linux64"
#else
#define C4_OS "linux"
#endif
#elif defined(__APPLE__)
#define C4_OS "mac"
#else
#define C4_OS "unknown";
#endif

#ifdef C4ENGINE

#ifndef HAVE_CONFIG_H
// different debugrec options
//#define DEBUGREC

// define directive STAT here to activate statistics
#undef STAT

#endif // HAVE_CONFIG_H

#ifdef DEBUGREC
#define DEBUGREC_SCRIPT
#define DEBUGREC_START_FRAME 0
#define DEBUGREC_PXS
#define DEBUGREC_OBJCOM
#define DEBUGREC_MATSCAN
//#define DEBUGREC_RECRUITMENT
#define DEBUGREC_MENU
#define DEBUGREC_OCF
#endif

// solidmask debugging
//#define SOLIDMASK_DEBUG

// fmod
#if defined(USE_FMOD) && !defined(HAVE_SDL_MIXER)
#define C4SOUND_USE_FMOD
#endif

#ifdef _WIN32
// resources
#include "res/engine_resource.h"
#endif // _WIN32

#endif // C4ENGINE

#include <Standard.h>
#include <CStdFile.h>
#include <Fixed.h>
#include <StdAdaptors.h>
#include <StdBuf.h>
#include <StdConfig.h>
#include <StdCompiler.h>
#include <StdDDraw2.h>
#include <StdFacet.h>
#include <StdFile.h>
#include <StdFont.h>
#include <StdMarkup.h>
#include <StdPNG.h>
#include <StdResStr2.h>
#include <StdSurface2.h>

#include "C4Id.h"
#include "C4Prototypes.h"
#include "C4Constants.h"

#ifdef _WIN32
#include <mmsystem.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <stdarg.h>

#if defined(BIG_C4INCLUDE) && defined(C4ENGINE)
#include "C4Application.h"
#include "C4Aul.h"
#include "C4ChatDlg.h"
#include "C4Client.h"
#include "C4Command.h"
#include "C4ComponentHost.h"
#include "C4Components.h"
#include "C4Config.h"
#include "C4Console.h"
#include "C4Control.h"
#include "C4DefGraphics.h"
#include "C4Def.h"
#include "C4DevmodeDlg.h"
#include "C4EditCursor.h"
#include "C4Effects.h"
#include "C4Extra.h"
#include "C4FacetEx.h"
#include "C4Facet.h"
#include "C4FileClasses.h"
#include "C4FileSelDlg.h"
#include "C4FindObject.h"
#include "C4FogOfWar.h"
#include "C4Fonts.h"
#include "C4FullScreen.h"
#include "C4GameControl.h"
#include "C4GameControlNetwork.h"
#include "C4GameDialogs.h"
#include "C4Game.h"
#include "C4GameLobby.h"
#include "C4GameMessage.h"
#include "C4GameObjects.h"
#include "C4GameOptions.h"
#include "C4GameOverDlg.h"
#include "C4GamePadCon.h"
#include "C4GameSave.h"
#include "C4GraphicsResource.h"
#include "C4GraphicsSystem.h"
#include "C4Group.h"
#include "C4GroupSet.h"
#include "C4Gui.h"
#include "C4IDList.h"
#include "C4InfoCore.h"
#include "C4InputValidation.h"
#include "C4KeyboardInput.h"
#include "C4Landscape.h"
#include "C4LangStringTable.h"
#include "C4Language.h"
#include "C4League.h"
#include "C4LoaderScreen.h"
#include "C4LogBuf.h"
#include "C4Log.h"
#include "C4MapCreatorS2.h"
#include "C4Map.h"
#include "C4MassMover.h"
#include "C4Material.h"
#include "C4Menu.h"
#include "C4MessageBoard.h"
#include "C4MessageInput.h"
#include "C4MouseControl.h"
#include "C4MusicFile.h"
#include "C4MusicSystem.h"
#include "C4NameList.h"
#include "C4NetIO.h"
#include "C4Network2Client.h"
#include "C4Network2Dialogs.h"
#include "C4Network2Discover.h"
#include "C4Network2.h"
#include "C4Network2IO.h"
#include "C4Network2Players.h"
#include "C4Network2Res.h"
#include "C4Network2Stats.h"
#include "C4ObjectCom.h"
#include "C4Object.h"
#include "C4ObjectInfo.h"
#include "C4ObjectInfoList.h"
#include "C4ObjectList.h"
#include "C4ObjectMenu.h"
#include "C4PacketBase.h"
#include "C4Particles.h"
#include "C4PathFinder.h"
#include "C4Physics.h"
#include "C4Player.h"
#include "C4PlayerInfo.h"
#include "C4PlayerInfoListBox.h"
#include "C4PlayerList.h"
#include "C4PropertyDlg.h"
#include "C4PXS.h"
#include "C4Random.h"
#include "C4RankSystem.h"
#include "C4Record.h"
#include "C4Region.h"
#include "C4RoundResults.h"
#include "C4RTF.h"
#include "C4Scenario.h"
#include "C4Scoreboard.h"
#include "C4Script.h"
#include "C4ScriptHost.h"
#include "C4Sector.h"
#include "C4Shape.h"
#include "C4Sky.h"
#include "C4SolidMask.h"
#include "C4SoundSystem.h"
#include "C4Startup.h"
#include "C4StartupMainDlg.h"
#include "C4StartupNetDlg.h"
#include "C4StartupOptionsDlg.h"
#include "C4StartupAboutDlg.h"
#include "C4StartupPlrSelDlg.h"
#include "C4StartupScenSelDlg.h"
#include "C4Stat.h"
#include "C4StringTable.h"
#include "C4SurfaceFile.h"
#include "C4Surface.h"
#include "C4Teams.h"
#include "C4Texture.h"
#include "C4ToolsDlg.h"
#include "C4TransferZone.h"
#include "C4Update.h"
#include "C4UpperBoard.h"
#include "C4UserMessages.h"
#include "C4Value.h"
#include "C4ValueList.h"
#include "C4ValueMap.h"
#include "C4Video.h"
#include "C4Viewport.h"
#include "C4Weather.h"
#include "C4Wrappers.h"
#endif
