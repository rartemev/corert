// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <stdlib.h>

#include "dllexport.h"

#ifdef _X86_
#ifdef PLATFORM_UNIX
#define DEFAULT_CALL_CONV __cdecl
#else
#define DEFAULT_CALL_CONV __stdcall
#endif
#else
#define DEFAULT_CALL_CONV
#define __cdecl
#define __stdcall
#endif

class JitConfigProvider
{
public:
    virtual int DEFAULT_CALL_CONV getIntConfigValue(
        const wchar_t* name, 
        int defaultValue
        ) = 0;

    virtual int DEFAULT_CALL_CONV getStringConfigValue(
        const wchar_t* name,
        wchar_t* retBuffer,
        int retBufferLength
        ) = 0;
};

// Native implementation of the JIT host.
// The native implementation calls into JitConfigProvider (implemented on the managed side) to get the actual
// configuration values.
// This dance is necessary because RyuJIT calls into the JitHost as part of the process shutdown (to free up
// strings). JitHost therefore can't be implemented in managed code (because managed runtime might have
// already shut down).

class JitHost
{
    JitConfigProvider* pConfigProvider;

public:
    JitHost(JitConfigProvider* pConfigProvider)
        : pConfigProvider(pConfigProvider) { }

    virtual void* allocateMemory(size_t size, bool usePageAllocator = false)
    {
        return malloc(size);
    }

    virtual void freeMemory(void* block, bool usePageAllocator = false)
    {
        free(block);
    }

    virtual int getIntConfigValue(
        const wchar_t* name, 
        int defaultValue
        )
    {
        return pConfigProvider->getIntConfigValue(name, defaultValue);
    }

    virtual const wchar_t* getStringConfigValue(
        const wchar_t* name
        )
    {
        // Find out the required length of the buffer
        int numCharacters = pConfigProvider->getStringConfigValue(name, nullptr, 0);
        if (numCharacters == 0)
            return nullptr;

        // Allocate extra char for the null terminator
        wchar_t* retBuffer = (wchar_t*)calloc(numCharacters + 1, sizeof(wchar_t));
        pConfigProvider->getStringConfigValue(name, retBuffer, numCharacters);

        return retBuffer;
    }

    virtual void freeStringConfigValue(
        wchar_t* value
        )
    {
        free(value);
    }
};

DLL_EXPORT void* GetJitHost(JitConfigProvider* pConfigProvider)
{
    return new JitHost(pConfigProvider);
}
