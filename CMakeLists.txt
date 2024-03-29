# LegacyClonk
#
# Copyright (c) 2017, The LegacyClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

cmake_minimum_required(VERSION 3.8)

if(WIN32)
	set(CMAKE_SYSTEM_VERSION 6.1 CACHE TYPE INTERNAL FORCE) # Windows 7
endif()

project("LegacyClonk")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)
include(CMakeDependentOption)

# Make sure headers of the project are found
include_directories(src)

# Additional directory to search for library files
set(EXTRA_DEPS_DIR "deps" CACHE PATH "Additional directory to search for libraries and headers")
include_directories(${EXTRA_DEPS_DIR}/include)
list(APPEND CMAKE_PREFIX_PATH ${EXTRA_DEPS_DIR})

# Check which header files and functions are present

CHECK_INCLUDE_FILE_CXX(direct.h           HAVE_DIRECT_H)
CHECK_INCLUDE_FILE_CXX(execinfo.h         HAVE_EXECINFO_H)
if(NOT WIN32)
	CHECK_INCLUDE_FILE_CXX(iconv.h            HAVE_ICONV)
endif()
CHECK_INCLUDE_FILE_CXX(io.h               HAVE_IO_H)
CHECK_INCLUDE_FILE_CXX(langinfo.h         HAVE_LANGINFO_H)
CHECK_INCLUDE_FILE_CXX(locale.h           HAVE_LOCALE_H)
CHECK_INCLUDE_FILE_CXX(share.h            HAVE_SHARE_H)
CHECK_INCLUDE_FILE_CXX(signal.h           HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE_CXX(stdint.h           HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CXX(sys/inotify.h      HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h       HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CXX(sys/stat.h         HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CXX(sys/syscall.h      HAVE_SYS_SYSCALL_H)
CHECK_INCLUDE_FILE_CXX(sys/types.h        HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CXX(unistd.h           HAVE_UNISTD_H)
CHECK_CXX_SYMBOL_EXISTS(vasprintf stdio.h HAVE_VASPRINTF)

# Define options

option(DEBUGREC "Write additional debug control to records" OFF)
option(USE_CONSOLE "Dedicated server mode (compile as pure console application)" OFF)

# ENABLE_SOUND
CMAKE_DEPENDENT_OPTION(ENABLE_SOUND "Compile with sound support" ON
	"NOT USE_CONSOLE" OFF)
# USE_FMOD
CMAKE_DEPENDENT_OPTION(USE_FMOD "Whether FMOD shall be used" ON
	"ENABLE_SOUND AND WIN32" OFF)
# USE_MP3
CMAKE_DEPENDENT_OPTION(USE_MP3 "Enable MP3 support" OFF
	"ENABLE_SOUND" OFF)
# USE_SDL_MAINLOOP
if (APPLE)
	set(USE_SDL_MAINLOOP_DEFAULT ON)
else ()
	set(USE_SDL_MAINLOOP_DEFAULT OFF)
endif ()
CMAKE_DEPENDENT_OPTION(USE_SDL_MAINLOOP "Use SDL instead of X11 or Win32" ${USE_SDL_MAINLOOP_DEFAULT}
	"NOT USE_CONSOLE AND NOT WIN32" OFF)
# USE_X11
CMAKE_DEPENDENT_OPTION(USE_X11 "Use X11" ON
	"NOT USE_CONSOLE AND NOT USE_SDL_MAINLOOP AND NOT APPLE AND NOT WIN32" OFF)
# USE_WIC
CMAKE_DEPENDENT_OPTION(USE_WIC "Use WIC instead of libjpeg and libpng" ON
	"WIN32" OFF)
# WITH_DEVELOPER_MODE
CMAKE_DEPENDENT_OPTION(WITH_DEVELOPER_MODE "Use GTK for the developer mode" OFF
	"NOT USE_CONSOLE" OFF)

# Check whether SDL_mixer should be used
if (ENABLE_SOUND AND NOT WIN32)
	set(USE_SDL_MIXER ON)
endif ()

# Use Microsoft CryptoAPI instead of OpenSSL when compiling for Windows
if (WIN32)
	set(USE_MSCAPI ON)
endif ()

# Adds the lines of file LIST_FILE to variable TARGET_LIST
function(append_filelist TARGET_LIST LIST_FILE)
	file(STRINGS "cmake/filelists/${LIST_FILE}" FILE_CONTENTS)
	list(APPEND ${TARGET_LIST} ${FILE_CONTENTS})
	set(${TARGET_LIST} "${${TARGET_LIST}}" PARENT_SCOPE)
endfunction()

# Add engine target

append_filelist(CLONK_SOURCES Engine.txt)

if (WIN32)
	append_filelist(CLONK_SOURCES EngineWin32.txt)
elseif (APPLE)
	append_filelist(CLONK_SOURCES EngineApple.txt)
endif ()

if (USE_CONSOLE)
	add_executable(clonk                     ${CLONK_SOURCES})
else ()
	add_executable(clonk WIN32 MACOSX_BUNDLE ${CLONK_SOURCES})
endif ()
target_compile_definitions(clonk PRIVATE C4ENGINE)

# Add c4group target

append_filelist(C4GROUP_SOURCES C4Group.txt)
if (WIN32)
	list(APPEND C4GROUP_SOURCES src/c4group_cmdl.cpp)
else ()
	list(APPEND C4GROUP_SOURCES src/c4group_ng.cpp)
endif ()
add_executable(c4group ${C4GROUP_SOURCES})
target_link_libraries(c4group standard)

# Add libstandard target

append_filelist(LIBSTANDARD_SOURCES Std.txt)

if (USE_CONSOLE AND NOT WIN32)
	append_filelist(LIBSTANDARD_SOURCES StdConsoleNonWin32.txt)
endif ()

if (USE_MSCAPI)
	list(APPEND LIBSTANDARD_SOURCES src/StdSha1MSCapi.cpp)
else ()
	list(APPEND LIBSTANDARD_SOURCES src/StdSha1OpenSSL.cpp)
endif ()

if (USE_SDL_MAINLOOP)
	append_filelist(LIBSTANDARD_SOURCES StdSdl.txt)
endif ()

if (USE_X11)
	append_filelist(LIBSTANDARD_SOURCES StdX.txt)
endif ()

if (USE_WIC)
	list(APPEND LIBSTANDARD_SOURCES
		src/StdJpegWic.cpp src/StdPNGWic.cpp src/StdWic.cpp src/StdWic.h)
else ()
	list(APPEND LIBSTANDARD_SOURCES
		src/StdJpegLibjpeg.cpp src/StdPNGLibpng.cpp)
endif ()

if (WIN32)
	append_filelist(LIBSTANDARD_SOURCES StdWin32.txt)
endif ()

if (WITH_DEVELOPER_MODE)
	append_filelist(LIBSTANDARD_SOURCES StdGtk.txt)
endif ()

add_library(standard STATIC ${LIBSTANDARD_SOURCES})
target_compile_definitions(standard PRIVATE C4ENGINE)
if (WIN32)
	target_link_libraries(standard vfw32)
endif ()
target_link_libraries(clonk standard)

# Link libraries

# Link FMOD
if (USE_FMOD)
 	find_library(FMOD_LIBRARY fmodvc)
	if (NOT FMOD_LIBRARY)
		message(FATAL_ERROR "Cannot find FMOD library")
	endif ()
	target_link_libraries(standard ${FMOD_LIBRARY})
endif ()

# Link Freetype
if (NOT USE_CONSOLE)
	find_package(Freetype REQUIRED)
	target_include_directories(standard PRIVATE ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(standard ${FREETYPE_LIBRARIES})
	set(HAVE_FREETYPE ${FREETYPE_FOUND})
endif ()

# Link GTK2
if (WITH_DEVELOPER_MODE)
	set(WITH_GLIB 1)
	find_package(GTK2 2.4 REQUIRED gtk)
	target_include_directories(clonk    PRIVATE ${GTK2_INCLUDE_DIRS})
	target_include_directories(standard PRIVATE ${GTK2_INCLUDE_DIRS})
	target_link_libraries(clonk    ${GTK2_LIBRARIES})
	target_link_libraries(standard ${GTK2_LIBRARIES})
endif ()

# Link iconv
if (HAVE_ICONV AND APPLE)
	target_link_libraries(clonk iconv)
endif ()

# Link libjpeg
if (NOT USE_WIC)
	find_package(JPEG REQUIRED)
	target_include_directories(standard PRIVATE ${JPEG_INCLUDE_DIR})
	target_link_libraries(standard ${JPEG_LIBRARIES})
endif ()

# Link OpenGL and GLEW
if (NOT USE_CONSOLE)
	find_package(OpenGL REQUIRED)
	if (NOT TARGET OpenGL::GLU)
		message(FATAL_ERROR "Cannot find GLU library")
	endif ()
	find_package(GLEW REQUIRED)
	target_link_libraries(standard OpenGL::GL OpenGL::GLU GLEW::GLEW)
endif ()

# Link OpenSSL crypto library
if (NOT USE_MSCAPI)
	find_package(OpenSSL)
	if (NOT TARGET OpenSSL::Crypto)
		message(FATAL_ERROR "Cannot find OpenSSL crypto library")
	endif ()
	target_link_libraries(standard OpenSSL::Crypto)
endif ()

# Link libpng
if (NOT USE_WIC)
	find_package(PNG REQUIRED)
	target_link_libraries(standard PNG::PNG)
endif ()

# Link SDL
if (USE_SDL_MAINLOOP OR USE_SDL_MIXER)
	find_package(SDL REQUIRED)
	if (USE_SDL_MAINLOOP)
		set(HAVE_SDL ON)
	endif ()
	target_include_directories(standard PUBLIC ${SDL_INCLUDE_DIR})
	target_link_libraries(standard ${SDL_LIBRARY})
endif ()

# Link SDL_mixer
if (USE_SDL_MIXER)
	find_package(SDL_mixer REQUIRED)
	target_include_directories(clonk PRIVATE ${SDL_MIXER_INCLUDE_DIRS})
	target_link_libraries(clonk ${SDL_MIXER_LIBRARIES})
	set(HAVE_LIBSDL_MIXER ON)
endif ()

# Link Windows Imaging Component
if (USE_WIC)
	target_link_libraries(standard windowscodecs)
endif ()

# Link thread library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
if (THREADS_FOUND)
	target_link_libraries(standard Threads::Threads)
endif ()
set(HAVE_PTHREAD ${CMAKE_USE_PTHREADS_INIT})

# Link Windows libraries
if (WIN32)
	target_link_libraries(clonk winmm ws2_32)
endif ()

# Link X11
if (USE_X11)
	find_package(X11 REQUIRED)
	if (NOT X11_FOUND)
		message(FATAL_ERROR "libX11 not found.")
	endif ()
	if (NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "libXpm not found.")
	endif ()
	if (NOT X11_xf86vmode_FOUND)
		message(FATAL_ERROR "XF86VidMode not found.")
	endif ()
	target_include_directories(standard PRIVATE
		${X11_X11_INCLUDE_PATH} ${X11_Xpm_INCLUDE_PATH} ${X11_xf86vmode_INCLUDE_PATH})
	target_link_libraries(standard
		${X11_X11_LIB}          ${X11_Xpm_LIB}          ${X11_Xxf86vm_LIB})
endif ()

# Link zlib
find_package(ZLIB REQUIRED)
target_link_libraries(standard ZLIB::ZLIB)

# Define _DEBUG or NDEBUG depending on project configuration (Already defined by MSVC)
if (NOT MSVC)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
		$<$<CONFIG:Debug>:_DEBUG>
		$<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
endif ()

# Get non-standard <cmath> constants (M_PI etc.)
if (MSVC)
	set(_USE_MATH_DEFINES ON)
endif ()

# Disable some Visual C++ warnings
if (MSVC)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
endif ()

# Create config.h and make sure it will be used and found
add_definitions(-DHAVE_CONFIG_H)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
