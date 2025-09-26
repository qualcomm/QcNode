// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_CLH
#define QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_CLH

KernelCode(

        __kernel void LetterboxNV12ToRGBMultiple(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                __global const int *roiPtr, int resizeHeight, int resizeWidth, int inputStride0,
                int inputPlane0Size, int inputStride1, int outputStride, int paddingValue ) {
            int i = get_global_id( 0 );
            int x = get_global_id( 1 );
            int y = get_global_id( 2 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + i * resizeHeight * outputStride +
                                  mad24( y, outputStride, x * 3 );
            int4 XYWH = vload4( 0, roiPtr + i * 4 );
            float inputRatio = (float) XYWH.s3 * native_recip( (float) XYWH.s2 );
            float outputRatio = (float) resizeHeight * native_recip( (float) resizeWidth );
            uchar3 RGB;

            if ( inputRatio < outputRatio )
            {
                if ( y < ( resizeWidth * inputRatio ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeWidth ) *
                                     (float) XYWH.s2 ) +
                              XYWH.s0;
                    int yIn = round( (float) y * native_recip( (float) resizeWidth ) *
                                     (float) XYWH.s2 ) +
                              XYWH.s1;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
            else
            {
                if ( x < ( resizeHeight * native_recip( inputRatio ) ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeHeight ) *
                                     (float) XYWH.s3 ) +
                              XYWH.s0;
                    int yIn = round( (float) y * native_recip( (float) resizeHeight ) *
                                     (float) XYWH.s3 ) +
                              XYWH.s1;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
        }

)

#endif   // QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_CLH
