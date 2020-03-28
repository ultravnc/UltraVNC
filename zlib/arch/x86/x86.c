/*
 * x86 feature check
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 * Author:
 *  Jim Kukunas
 * 
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "x86.h"

int x86_cpu_has_sse42 = 0;
int x86_cpu_has_pclmul = 0;

static void _x86_check_features(void);

#ifndef _MSC_VER
#include <pthread.h>
#include <cpuid.h>

pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;

void x86_check_features(void)
{
    pthread_once(&cpu_check_inited_once, _x86_check_features);
}

static void cpuid(int info, unsigned* eax, unsigned* ebx, unsigned* ecx, unsigned* edx) {
    unsigned int _eax;
    unsigned int _ebx;
    unsigned int _ecx;
    unsigned int _edx;
    __cpuid(info, _eax, _ebx, _ecx, _edx);
    *eax = _eax;
    *ebx = _ebx;
    *ecx = _ecx;
    *edx = _edx;
}
#else
#include <intrin.h>
#include <windows.h>
#include <stdint.h>

static volatile long once_control = 0;
static int fake_pthread_once(volatile long *once_control,
                             void (*init_routine)(void));

void x86_check_features(void)
{
    fake_pthread_once(&once_control, _x86_check_features);
}

/* Copied from "perftools_pthread_once" in tcmalloc */
static int fake_pthread_once(volatile long *once_control,
                             void (*init_routine)(void)) {
    // Try for a fast path first. Note: this should be an acquire semantics read
    // It is on x86 and x64, where Windows runs.
    if (*once_control != 1) {
        while (1) {
            switch (InterlockedCompareExchange(once_control, 2, 0)) {
                case 0:
                    init_routine();
                    InterlockedExchange(once_control, 1);
                    return 0;
                case 1:
                    // The initializer has already been executed
                    return 0;
                default:
                    // The initializer is being processed by another thread
                    SwitchToThread();
            }
        }
    }
    return 0;
}

static void cpuid(int info, unsigned* eax, unsigned* ebx, unsigned* ecx, unsigned* edx) {
    int registers[4];
    __cpuid(registers, info);

    *eax = (unsigned)registers[0];
    *ebx = (unsigned)registers[1];
    *ecx = (unsigned)registers[2];
    *edx = (unsigned)registers[3];
}
#endif  /* _MSC_VER */

static void _x86_check_features(void)
{
    unsigned eax, ebx, ecx, edx;
    cpuid(1 /*CPU_PROCINFO_AND_FEATUREBITS*/, &eax, &ebx, &ecx, &edx);

    x86_cpu_has_sse42 = ecx & 0x100000;
    // All known cpus from Intel and AMD with CLMUL also support SSE4.2
    x86_cpu_has_pclmul = ecx & 0x2;
}
