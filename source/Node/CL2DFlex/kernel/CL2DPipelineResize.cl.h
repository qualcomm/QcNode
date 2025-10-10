// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_CL2D_PIPELINE_RESIZE_CLH
#define QC_CL2D_PIPELINE_RESIZE_CLH

KernelCode(

        __kernel void ResizeNV12ToRGB( __global const uchar *srcPtr, int srcOffset,
                                       __global uchar *dstPtr, int dstOffset, int inputHeight,
                                       int inputWidth, int resizeHeight, int resizeWidth,
                                       int inputStride0, int inputPlane0Size, int inputStride1,
                                       int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );

            int xIn = round( (float) ( x + roiX ) * native_recip( (float) resizeWidth ) *
                             (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) * native_recip( (float) resizeHeight ) *
                             (float) inputHeight );
            int yPtr = mad24( yIn, inputStride0, xIn );
            int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
            float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
            float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            uchar3 RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
            vstore3( RGB, 0, dst );
        }

        __kernel void ResizeUYVYToRGB( __global const uchar *srcPtr, int srcOffset,
                                       __global uchar *dstPtr, int dstOffset, int inputHeight,
                                       int inputWidth, int resizeHeight, int resizeWidth,
                                       int inputStride, int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *src = srcPtr + srcOffset;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );

            int xIn = round( (float) ( x + roiX ) * native_recip( (float) resizeWidth ) *
                             (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) * native_recip( (float) resizeHeight ) *
                             (float) inputHeight );
            int yPtr = mad24( yIn, inputStride, xIn << 1 ) + 1;
            int uPtr = mad24( yIn, inputStride, ( xIn / 2 ) * 4 );
            float Y = max( 0, src[yPtr] - 16 ) * coeffY;
            float3 UV3 = convert_float3( vload3( 0, src + uPtr ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV3.s0, UV3.s2, UV3.s0, UV3.s2 );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            uchar3 RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
            vstore3( RGB, 0, dst );
        }

        __kernel void ResizeUYVYToNV12( __global const uchar *srcPtr, int srcOffset,
                                        __global uchar *dstPtr, int dstOffset, int inputHeight,
                                        int inputWidth, int resizeHeight, int resizeWidth,
                                        int inputStride, int outputStride0, int outputPlane0Size,
                                        int outputStride1, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global uchar *ydst = dstPtr + dstOffset + mad24( y, outputStride0, x );
            int xIn1 = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn1 = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int yPtr = mad24( yIn1, inputStride, xIn1 << 1 ) + 1;
            ydst[0] = srcPtr[yPtr + srcOffset];
            if ( ( x < resizeWidth / 2 ) && ( y < resizeHeight / 2 ) )
            {
                __global uchar *udst =
                        dstPtr + dstOffset + outputPlane0Size + mad24( y, outputStride1, x << 1 );
                int xIn2 = round( (float) ( ( x + roiX / 2 ) << 1 ) / (float) resizeWidth *
                                  (float) inputWidth );
                int yIn2 = round( (float) ( ( y + roiY / 2 ) << 1 ) / (float) resizeHeight *
                                  (float) inputHeight );
                int uPtr = mad24( yIn2, inputStride, ( xIn2 / 2 ) * 4 );
                udst[0] = srcPtr[uPtr + srcOffset];
                udst[1] = srcPtr[uPtr + srcOffset + 2];
            }
        }

        __kernel void ResizeRGBToRGB( __global const uchar *srcPtr, int srcOffset,
                                      __global uchar *dstPtr, int dstOffset, int inputHeight,
                                      int inputWidth, int resizeHeight, int resizeWidth,
                                      int inputStride, int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            int xIn = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int ptr = mad24( yIn, inputStride, xIn * 3 );
            dst[0] = srcPtr[srcOffset + ptr + 0];
            dst[1] = srcPtr[srcOffset + ptr + 1];
            dst[2] = srcPtr[srcOffset + ptr + 2];
        }

        __kernel void ResizeNV12ToNV12( __global const uchar *srcPtr, int srcOffset,
                                        __global uchar *dstPtr, int dstOffset, int inputHeight,
                                        int inputWidth, int resizeHeight, int resizeWidth,
                                        int inputStride0, int inputPlane0Size, int inputStride1,
                                        int outputStride0, int outputPlane0Size, int outputStride1,
                                        int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global uchar *ydst = dstPtr + dstOffset + mad24( y, outputStride0, x );
            int xIn1 = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn1 = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int yPtr = mad24( yIn1, inputStride0, xIn1 );
            ydst[0] = srcPtr[srcOffset + yPtr];
            if ( ( x < resizeWidth / 2 ) && ( y < resizeHeight / 2 ) )
            {
                __global uchar *udst =
                        dstPtr + dstOffset + outputPlane0Size + mad24( y, outputStride1, x << 1 );
                int xIn2 = round( (float) ( x + roiX / 2 ) / (float) resizeWidth *
                                  (float) inputWidth );
                int yIn2 = round( (float) ( y + roiY / 2 ) / (float) resizeHeight *
                                  (float) inputHeight );
                int uPtr = mad24( yIn2, inputStride1, xIn2 << 1 );
                udst[0] = srcPtr[srcOffset + inputPlane0Size + uPtr];
                udst[1] = srcPtr[srcOffset + inputPlane0Size + uPtr + 1];
            }
        }

)

#endif   // QC_CL2D_PIPELINE_RESIZE_CLH
