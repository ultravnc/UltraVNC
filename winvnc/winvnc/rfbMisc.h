// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//


#include <algorithm>

#undef min
#undef max

#ifndef __RFB_MISC_INCLUDED__
#define __RFB_MISC_INCLUDED__

// Some platforms (e.g. Windows) include max() and min() functions
// in their standard headers.
// These macros are pdefined only when standard equivalents cannot
// be found.

#ifdef WIN32

// WIN32-ONLY PROFILING CODE
//
// CpuTime and CpuTimer provide a simple way to profile particular
// sections of code
//
// Use one CpuTime object per task to be profiled. CpuTime instances
// maintain a cumulative total of time spent in user and kernel space
// by threads.
// When a CpuTime object is created, a label must be specified to
// identify the task being profiled.
// When the object is destroyed, it will print debugging information
// containing the user and kernel times accumulated.
//
// Place a CpuTimer object in each section of code which is to be
// profiled. When the object is created, it snapshots the current
// kernel and user times and stores them. These are used when the
// object is destroyed to establish how much time has elapsed in the
// intervening period. The accumulated time is then added to the
// associated CpuTime object.
//
// This code works only on platforms providing __int64

namespace rfb {
	class CpuTime {
	public:
		CpuTime(const char* name)
			: timer_name(_strdup(name)),
			kernel_time(0), user_time(0), iterations(0), max_user_time(0) {}
		~CpuTime() {
			vnclog.Print(0, "timer %s : %I64ums (krnl), %I64ums (user), %I64uus (user-max) (%I64u its)\n",
				timer_name, kernel_time / 10000, user_time / 10000, max_user_time / 10,
				iterations);
			delete[] timer_name;
		}
		char* timer_name;
		__int64 kernel_time;
		__int64 user_time;
		__int64 iterations;
		__int64 max_user_time;
	};

	class CpuTimer {
	public:
		inline CpuTimer(CpuTime& ct) : cputime(ct) {
			FILETIME create_time{}, end_time{};
			if (!GetThreadTimes(GetCurrentThread(),
				&create_time, &end_time,
				(LPFILETIME)&start_kernel_time,
				(LPFILETIME)&start_user_time)) {
				abort();
			}
		}
		inline ~CpuTimer() {
			FILETIME create_time{}, end_time{};
			__int64 end_kernel_time{}, end_user_time{};
			if (!GetThreadTimes(GetCurrentThread(),
				&create_time, &end_time,
				(LPFILETIME)&end_kernel_time,
				(LPFILETIME)&end_user_time)) {
				abort();
			}
			cputime.kernel_time += end_kernel_time - start_kernel_time;
			cputime.user_time += end_user_time - start_user_time;
			if (end_user_time - start_user_time > cputime.max_user_time) {
				cputime.max_user_time = end_user_time - start_user_time;
			}
			cputime.iterations++;
		}
	private:
		CpuTime& cputime;
		__int64 start_kernel_time;
		__int64 start_user_time;
	};
}

#endif

#endif // __RFB_MISC_INCLUDED__
