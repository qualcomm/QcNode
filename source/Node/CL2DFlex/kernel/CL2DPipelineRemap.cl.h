// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2D_PIPELINE_REMAP_CLH
#define QC_CL2D_PIPELINE_REMAP_CLH

KernelCode(

        __kernel void RemapNV12ToRGB( __global const uchar *srcPtr, int srcOffset,
                                      __global uchar *dstPtr, int dstOffset, int inputHeight,
                                      int inputWidth, int resizeHeight, int resizeWidth,
                                      int inputStride0, int inputPlane0Size, int inputStride1,
                                      int outputStride, int roiX, int roiY,
                                      __global const float *mapX, __global const float *mapY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            uchar3 RGB;
            int xIn = round( mapX[x + y * resizeWidth] );
            xIn = max( 0, xIn );
            xIn = min( inputWidth - 1, xIn );
            xIn = roiX + xIn;
            int yIn = round( mapY[x + y * resizeWidth] );
            yIn = max( 0, yIn );
            yIn = min( inputHeight - 1, yIn );
            yIn = roiY + yIn;
            int yPtr = mad24( yIn, inputStride0, xIn );
            int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
            float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
            float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
            float4 UV4 = (float4) ( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            RGB = convert_uchar3_sat( (float3) ( UV4.s3, UV4.s1, UV4.s0 ) );
            vstore3( RGB, 0, dst );
        }

        __kernel void RemapNV12ToBGR( __global const uchar *srcPtr, int srcOffset,
                                      __global uchar *dstPtr, int dstOffset, int inputHeight,
                                      int inputWidth, int resizeHeight, int resizeWidth,
                                      int inputStride0, int inputPlane0Size, int inputStride1,
                                      int outputStride, int roiX, int roiY,
                                      __global const float *mapX, __global const float *mapY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            uchar3 BGR;
            int xIn = round( mapX[x + y * resizeWidth] );
            xIn = max( 0, xIn );
            xIn = min( inputWidth - 1, xIn );
            xIn = roiX + xIn;
            int yIn = round( mapY[x + y * resizeWidth] );
            yIn = max( 0, yIn );
            yIn = min( inputHeight - 1, yIn );
            yIn = roiY + yIn;
            int yPtr = mad24( yIn, inputStride0, xIn );
            int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
            float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
            float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
            float4 UV4 = (float4) ( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            BGR = convert_uchar3_sat( (float3) ( UV4.s0, UV4.s1, UV4.s3 ) );
            vstore3( BGR, 0, dst );
        }

)

#endif   // QC_CL2D_PIPELINE_REMAP_CLH
