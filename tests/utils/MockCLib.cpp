// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "MockCLib.hpp"
#include <cstddef>
#include <dlfcn.h>
#include <new>

extern "C"
{
    void *malloc( size_t size );
} /* extern "C" */

typedef void *( *malloc_fnc_t )( size_t size );
typedef void *( *calloc_fnc_t )( size_t nitems, size_t size );

static malloc_fnc_t malloc_fnc = nullptr;
static calloc_fnc_t calloc_fnc = nullptr;

static int s_whenMallocToReturnNull = 0;
static int s_whenCallocToReturnNull = 0;

void MockC_MallocCtrl( int whenToReturnNull )
{
    s_whenMallocToReturnNull = whenToReturnNull;
}

void MockC_CallocCtrl( int whenToReturnNull )
{
    s_whenCallocToReturnNull = whenToReturnNull;
}

void *malloc( size_t size )
{
    void *pData = nullptr;
    bool bReturnNull = false;

    if ( nullptr == malloc_fnc )
    {
        malloc_fnc = (malloc_fnc_t) dlsym( RTLD_NEXT, "malloc" );
        calloc_fnc = (calloc_fnc_t) dlsym( RTLD_NEXT, "calloc" );
    }

    if ( s_whenMallocToReturnNull > 0 )
    {
        s_whenMallocToReturnNull--;
        if ( 0 == s_whenMallocToReturnNull )
        {
            bReturnNull = true;
        }
    }

    if ( true == bReturnNull )
    {
        pData = nullptr;
    }
    else
    {
        pData = malloc_fnc( size );
    }
    return pData;
}


void *calloc( size_t nitems, size_t size )
{
    void *pData = nullptr;
    bool bReturnNull = false;

    if ( nullptr == malloc_fnc )
    {
        malloc_fnc = (malloc_fnc_t) dlsym( RTLD_NEXT, "malloc" );
        calloc_fnc = (calloc_fnc_t) dlsym( RTLD_NEXT, "calloc" );
    }

    if ( s_whenCallocToReturnNull > 0 )
    {
        s_whenCallocToReturnNull--;
        if ( 0 == s_whenCallocToReturnNull )
        {
            bReturnNull = true;
        }
    }

    if ( true == bReturnNull )
    {
        pData = nullptr;
    }
    else
    {
        pData = calloc_fnc( nitems, size );
    }
    return pData;
}
