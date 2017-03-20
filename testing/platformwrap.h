#pragma once

// Place for random platform dependent pieces used by test code


// ----------------------------------------------------------------------
// Basic platform macros

#if defined(_MSC_VER)
#	define PLATFORM_MICROSOFT 1
#	if defined(_XBOX_ONE)
#		define PLATFORM_XBOXONE 1
#	else
#		define PLATFORM_WINDOWS 1
#	endif
#endif

#if defined(__APPLE__)
#	include <TargetConditionals.h>
#	define PLATFORM_APPLE 1
#	if TARGET_OS_IPHONE
#		define PLATFORM_IOS 1
#	else
#		define PLATFORM_MAC 1
#	endif
#endif

#if defined(EMSCRIPTEN)
#	define PLATFORM_WEBGL 1
#endif

#if defined(__ORBIS__)
#	define PLATFORM_PS4 1
#endif


// ------------------------------------------------------------------------------------
// Misc

#if PLATFORM_MICROSOFT
#	define _CRT_SECURE_NO_WARNINGS
#	include <Windows.h>
#endif

#if PLATFORM_PS4
#include <kernel.h>
size_t sceLibcHeapSize = 1000*1024*1024;		// default libc heap size is too small
unsigned int sceLibcHeapExtendedAlloc = 1;
#endif


// ------------------------------------------------------------------------------------
// Timer code - TimerBegin and TimerEnd functions

#if PLATFORM_MICROSOFT
	static LARGE_INTEGER s_Time0;
	static void TimerBegin()
	{
		QueryPerformanceCounter (&s_Time0);
	}
	static float TimerEnd()
	{
		static bool timerInited = false;
		static LARGE_INTEGER ticksPerSec;
		if (!timerInited) {
			QueryPerformanceFrequency(&ticksPerSec);
			timerInited = true;
		}
		LARGE_INTEGER ttt1;
		QueryPerformanceCounter (&ttt1);
		float timeTaken = float(double(ttt1.QuadPart-s_Time0.QuadPart) / double(ticksPerSec.QuadPart));
		return timeTaken;
	}

#elif PLATFORM_APPLE
    #include<mach/mach_time.h>
	static uint64_t s_Time0;
	static void TimerBegin()
	{
        s_Time0 = mach_absolute_time();
	}
	static float TimerEnd()
	{
        static bool timerInited = false;
        static double timerScale = 0;
        if (!timerInited)
        {
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            timerScale = (double)info.numer / (double)info.denom;
        }
        uint64_t t1 = mach_absolute_time();
        return (t1 - s_Time0) * timerScale / 1000000000.0;
	}

#elif PLATFORM_WEBGL
	#include "emscripten.h"
	static double s_Time0;
	static void TimerBegin()
	{
		s_Time0 = emscripten_get_now();
	}
	static float TimerEnd()
	{
		double t = emscripten_get_now();
		float timeTaken = (t - s_Time0) * 0.001;
		return timeTaken;
	}

#elif PLATFORM_PS4
	static SceKernelTimeval s_Time0;
	static void TimerBegin()
	{
		sceKernelGettimeofday(&s_Time0);
	}
	static float TimerEnd()
	{
		SceKernelTimeval ttt1;
		sceKernelGettimeofday( &ttt1 );
		SceKernelTimeval ttt2;
		timersub( &ttt1, &s_Time0, &ttt2 );
		float timeTaken = ttt2.tv_sec + ttt2.tv_usec * 1.0e-6f;
		
		return timeTaken;
	}

#else
	#error "Unknown platform, timer code missing"

#endif

