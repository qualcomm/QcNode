// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2D_PIPELINE_RESIZEMULTIPLE_CLH
#define QC_CL2D_PIPELINE_RESIZEMULTIPLE_CLH

KernelCode(

        __kernel void ResizeNV12ToRGBMultiple(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                __global const int *roiPtr, int resizeHeight, int resizeWidth, int inputStride0,
                int inputPlane0Size, int inputStride1, int outputStride ) {
            int i = get_global_id( 0 );
            int x = get_global_id( 1 );
            int y = get_global_id( 2 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + i * resizeHeight * outputStride +
                                  mad24( y, outputStride, x * 3 );
            int4 XYWH = vload4( 0, roiPtr + i * 4 );
            uchar3 RGB;
            int xIn = round( (float) x * native_recip( (float) resizeWidth ) * (float) XYWH.s2 ) +
                      XYWH.s0;
            int yIn = round( (float) y * native_recip( (float) resizeHeight ) * (float) XYWH.s3 ) +
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

)

#endif   // QC_CL2D_PIPELINE_RESIZEMULTIPLE_CLH
