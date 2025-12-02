// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void RgbToYuv( uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb )
{
    int32_t R = *rgb++;
    int32_t G = *rgb++;
    int32_t B = *rgb;

    // well known RGB to YUV algorithm
    *y = ( ( 66 * R + 129 * G + 25 * B + 128 ) >> 8 ) + 16;
    *u = ( ( -38 * R - 74 * G + 112 * B + 128 ) >> 8 ) + 128;
    *v = ( ( 112 * R - 94 * G - 18 * B + 128 ) >> 8 ) + 128;
}


static void RgbToYuv10( uint16_t *y, uint16_t *u, uint16_t *v, uint8_t *rgb )
{
    int32_t R = *rgb++;
    int32_t G = *rgb++;
    int32_t B = *rgb;

    *y = ( 66 * R + 129 * G + 25 * B + 128 ) + ( 16 << 8 );
    *u = ( -38 * R - 74 * G + 112 * B + 128 ) + ( 128 << 8 );
    *v = ( 112 * R - 94 * G - 18 * B + 128 ) + ( 128 << 8 );

    /* clear the 6 low bit as 0 */
    *y &= 0xFFC0;
    *u &= 0xFFC0;
    *v &= 0xFFC0;
}

static void *load( const char *path, size_t *sz )
{
    void *out;
    FILE *fp = fopen( path, "rb" );
    assert( fp );
    fseek( fp, 0, SEEK_END );
    *sz = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    out = malloc( *sz );
    fread( out, 1, *sz, fp );
    fclose( fp );
    return out;
}

static void save( const char *path, void *out, size_t sz )
{
    FILE *fp = fopen( path, "wb" );
    assert( fp );
    fwrite( out, 1, sz, fp );
    fclose( fp );
    printf( "save %s\n", path );
}

int rgb_to_uyvy( const char *rgbP, const char *yuvP, const size_t width, const size_t height )
{
    size_t isz = 0;
    uint8_t *input = (uint8_t *) load( rgbP, &isz );
    uint8_t *output = (uint8_t *) malloc( width * height * 3 );
    size_t yDataSize = width * height;
    size_t yuvDataSize = yDataSize * 2;
    size_t rgbDataSize = yDataSize * 3;
    assert( rgbDataSize == isz );

    uint8_t y0, u0, v0, y1, u1, v1;

    for ( int h = 0; h < height; h++ )
    {
        for ( int w = 0; w < width / 2; w++ )
        {
            RgbToYuv( &y0, &u0, &v0, input + ( h * width + w * 2 ) * 3 );
            RgbToYuv( &y1, &u1, &v1, input + ( h * width + w * 2 + 1 ) * 3 );

            *( output + ( h * ( width / 2 ) + w ) * 4 ) = u0;
            *( output + ( h * ( width / 2 ) + w ) * 4 + 1 ) = y0;
            *( output + ( h * ( width / 2 ) + w ) * 4 + 2 ) = v0;
            *( output + ( h * ( width / 2 ) + w ) * 4 + 3 ) = y1;
        }
    }

    save( yuvP, output, yuvDataSize );
    return 0;
}

int rgb_to_nv21( const char *rgbP, const char *yuvP, const size_t width, const size_t height )
{
    size_t isz = 0;
    uint8_t *input = (uint8_t *) load( rgbP, &isz );
    uint8_t *output = (uint8_t *) malloc( width * height * 3 );
    size_t yDataSize = width * height;
    size_t yuvDataSize = ( yDataSize * 3 ) / 2;
    size_t rgbDataSize = yDataSize * 3;
    assert( rgbDataSize == isz );

    size_t uvoffset = yDataSize;
    uint8_t u, v, y1, y2, y3, y4;

    for ( size_t i = 0, k = 0; i < yDataSize; i += 2, k += 2 )
    {
        RgbToYuv( &y1, &u, &v, input + i * 3 );
        RgbToYuv( &y2, &u, &v, input + ( i + 1 ) * 3 );
        RgbToYuv( &y3, &u, &v, input + ( width + i ) * 3 );
        RgbToYuv( &y4, &u, &v, input + ( width + i + 1 ) * 3 );

        *( output + i ) = y1;
        *( output + i + 1 ) = y2;
        *( output + width + i ) = y3;
        *( output + width + i + 1 ) = y4;
        *( output + uvoffset + k ) = v;
        *( output + uvoffset + k + 1 ) = u;

        if ( ( ( i + 2 ) % width ) == 0 ) i += width;
    }

    save( yuvP, output, yuvDataSize );
    return 0;
}

int rgb_to_nv12( const char *rgbP, const char *yuvP, const size_t width, const size_t height )
{
    size_t isz = 0;
    uint8_t *input = (uint8_t *) load( rgbP, &isz );
    uint8_t *output = (uint8_t *) malloc( width * height * 3 );
    size_t yDataSize = width * height;
    size_t yuvDataSize = ( yDataSize * 3 ) / 2;
    size_t rgbDataSize = yDataSize * 3;
    assert( rgbDataSize == isz );

    size_t uvoffset = yDataSize;
    uint8_t u, v, y1, y2, y3, y4;

    for ( size_t i = 0, k = 0; i < yDataSize; i += 2, k += 2 )
    {
        RgbToYuv( &y1, &u, &v, input + i * 3 );
        RgbToYuv( &y2, &u, &v, input + ( i + 1 ) * 3 );
        RgbToYuv( &y3, &u, &v, input + ( width + i ) * 3 );
        RgbToYuv( &y4, &u, &v, input + ( width + i + 1 ) * 3 );
        *( output + i ) = y1;
        *( output + i + 1 ) = y2;
        *( output + width + i ) = y3;
        *( output + width + i + 1 ) = y4;
        *( output + uvoffset + k ) = u;
        *( output + uvoffset + k + 1 ) = v;

        if ( ( ( i + 2 ) % width ) == 0 ) i += width;
    }

    save( yuvP, output, yuvDataSize );
    return 0;
}


int rgb_to_p010( const char *rgbP, const char *yuvP, const size_t width, const size_t height )
{
    size_t isz = 0;
    uint8_t *input = (uint8_t *) load( rgbP, &isz );
    uint16_t *output = (uint16_t *) malloc( width * height * 3 * 2 );
    size_t yDataSize = width * height;
    size_t yuvDataSize = yDataSize * 3;
    size_t rgbDataSize = yDataSize * 3;
    assert( rgbDataSize == isz );

    size_t uvoffset = yDataSize;
    uint16_t u, v, y1, y2, y3, y4;

    for ( size_t i = 0, k = 0; i < yDataSize; i += 2, k += 2 )
    {
        RgbToYuv10( &y1, &u, &v, input + i * 3 );
        RgbToYuv10( &y2, &u, &v, input + ( i + 1 ) * 3 );
        RgbToYuv10( &y3, &u, &v, input + ( width + i ) * 3 );
        RgbToYuv10( &y4, &u, &v, input + ( width + i + 1 ) * 3 );
        *( output + i ) = y1;
        *( output + i + 1 ) = y2;
        *( output + width + i ) = y3;
        *( output + width + i + 1 ) = y4;
        *( output + uvoffset + k ) = u;
        *( output + uvoffset + k + 1 ) = v;

        if ( ( ( i + 2 ) % width ) == 0 ) i += width;
    }

    save( yuvP, output, yuvDataSize );
    return 0;
}

int main( int argc, char *argv[] )
{
    const char *targetColor = argv[1];
    const char *rgbP = argv[2];
    const char *yuvP = argv[3];
    const size_t width = (size_t) atoi( argv[4] );
    const size_t height = (size_t) atoi( argv[5] );

    if ( 0 == strcmp( targetColor, "uyvy" ) )
    {
        return rgb_to_uyvy( rgbP, yuvP, width, height );
    }
    else if ( 0 == strcmp( targetColor, "nv21" ) )
    {
        return rgb_to_nv21( rgbP, yuvP, width, height );
    }
    else if ( 0 == strcmp( targetColor, "nv12" ) )
    {
        return rgb_to_nv12( rgbP, yuvP, width, height );
    }
    else if ( 0 == strcmp( targetColor, "p010" ) )
    {
        return rgb_to_p010( rgbP, yuvP, width, height );
    }

    return -1;
}