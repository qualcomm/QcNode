// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "MockCLib.hpp"
#include <cstddef>
#include <dlfcn.h>
#include <stdint.h>

#define MOCKC_INIT_START_MAGIC 0x12345678
#define MOCKC_INIT_MAGIC 0xdeadbeef

extern "C"
{
    void *malloc( size_t size );
    void *calloc( size_t nitems, size_t size );
} /* extern "C" */


typedef void *( *malloc_fnc_t )( size_t size );
typedef void *( *calloc_fnc_t )( size_t nitems, size_t size );

static malloc_fnc_t malloc_fnc = nullptr;
static calloc_fnc_t calloc_fnc = nullptr;

static int s_whenMallocToReturnNull = 0;
static int s_whenCallocToReturnNull = 0;

static size_t s_sizeMallocToReturnNull = 0;
static size_t s_sizeCallocToReturnNull = 0;

static uint32_t s_initMagic = 0;

static void MockC_EnsureInit( void )
{
    if ( MOCKC_INIT_MAGIC != s_initMagic )
    {
        s_initMagic = MOCKC_INIT_START_MAGIC;
        malloc_fnc = (malloc_fnc_t) dlsym( RTLD_NEXT, "malloc" );
        calloc_fnc = (calloc_fnc_t) dlsym( RTLD_NEXT, "calloc" );
        s_whenMallocToReturnNull = 0;
        s_whenCallocToReturnNull = 0;
        s_sizeMallocToReturnNull = 0;
        s_sizeCallocToReturnNull = 0;
        s_initMagic = MOCKC_INIT_MAGIC;
    }
}

void MockC_MallocCtrlSize( size_t size )
{
    s_sizeMallocToReturnNull = size;
}

void MockC_MallocCtrl( int whenToReturnNull )
{
    s_whenMallocToReturnNull = whenToReturnNull;
}

void MockC_CallocCtrlSize( size_t size )
{
    s_sizeCallocToReturnNull = size;
}

void MockC_CallocCtrl( int whenToReturnNull )
{
    s_whenCallocToReturnNull = whenToReturnNull;
}

void *malloc( size_t size )
{
    void *pData = nullptr;
    bool bReturnNull = false;

    if ( MOCKC_INIT_START_MAGIC == s_initMagic )
    {
        /* dlsym may call malloc */
        int r = posix_memalign( &pData, 128, size );
        if ( 0 != r )
        {
            pData = nullptr;
        }
        return pData;
    }

    MockC_EnsureInit();

    if ( s_whenMallocToReturnNull > 0 )
    {
        s_whenMallocToReturnNull--;
        if ( 0 == s_whenMallocToReturnNull )
        {
            bReturnNull = true;
        }
    }

    if ( s_sizeMallocToReturnNull > 0 )
    {
        if ( size == s_sizeMallocToReturnNull )
        {
            bReturnNull = true;
            s_sizeMallocToReturnNull = 0;
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

    if ( MOCKC_INIT_START_MAGIC == s_initMagic )
    {
        /* dlsym may call calloc */
        int r = posix_memalign( &pData, 128, nitems * size );
        if ( 0 == r )
        {
            memset( pData, 0, nitems * size );
        }
        else
        {
            pData = nullptr;
        }
        return pData;
    }

    MockC_EnsureInit();

    if ( s_whenCallocToReturnNull > 0 )
    {
        s_whenCallocToReturnNull--;
        if ( 0 == s_whenCallocToReturnNull )
        {
            bReturnNull = true;
        }
    }

    if ( s_sizeCallocToReturnNull > 0 )
    {
        if ( ( size * nitems ) == s_sizeCallocToReturnNull )
        {
            bReturnNull = true;
            s_sizeCallocToReturnNull = 0;
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
